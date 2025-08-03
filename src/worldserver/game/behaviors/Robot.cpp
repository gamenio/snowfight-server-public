#include "Robot.h"

#include "game/utils/MathTools.h"
#include "game/world/ObjectMgr.h"
#include "game/world/ObjectAccessor.h"
#include "game/maps/BattleMap.h"
#include "game/movement/spline/MoveSpline.h"
#include "game/movement/spline/MoveSegment.h"
#include "game/ai/TrainingRobotAI.h"
#include "game/ai/SparringRobotAI.h"
#include "game/grids/ObjectSearcher.h"
#include "game/server/protocol/OpcodeHandler.h"
#include "game/world/World.h"
#include "Player.h"
#include "ItemBox.h"
#include "UnitHelper.h"
#include "Projectile.h"
#include "ObjectShapes.h"
#include "UnitLocator.h"

const int32 SIGHT_DISTANCE_IN_TILES					= (int32)std::ceil(SIGHT_DISTANCE / (float)TILE_WIDTH_HALF);

#define ATTACK_INTERVAL								100		// 攻击间隔时间。单位：毫秒
#define EXPECTED_NUMBER_OF_ATTACKS_MIN				2		// 连续攻击时期望的最小攻击次数
#define AIMING_ERROR_RANGE_MAX						80.f	// 瞄准误差最大范围。单位：points

#define NORMAL_ATTACK_MIN_INTERVAL					120		// 普通攻击最小间隔时间。单位：毫秒
#define	NORMAL_ATTACK_MAX_INTERVAL					150		// 普通攻击最大间隔时间。

#define CHARGE_START_STAMINA_RATIO					0.8f	// 开始蓄力时的体力比例
#define CHARGE_START_ATTACK_RANGE_RATIO				0.6f	// 开始蓄力时的攻击距离与目标距离的比例

#define HAND_DOWN_DELAY								500		// 放下手的延迟时间。单位：毫秒
#define MISS_DISTANCE								64.0f	// 未击中时雪球掠过玩家的距离

#define EXCLUDED_EXPLOR_AREA_EXPIRATION_TIME		30000	// 被排除的探索区的失效时间。单位：毫秒

// 当敌人在指定的攻击距离内时所采用的攻击方式
#define ATTACK_CHARGED_WITHIN_DIST					140.0f
#define ATTACK_FAST_WITHIN_DIST						80.0f

#define ENEMY_CRITICAL_THREAT_DISTANCE				140.f	// 敌人对自己构成严重威胁的距离
#define FAVORABLE_MAGICBEAN_MAX_DIFF				4		// 有利于自己的魔豆最大差值

#define ITEMS_TO_COLLECT_AND_TARGETED_ITEM_DISTANCE	100.f	// 要收集的物品与目标物品的距离

uint8 RobotTemplate::findStageByCombatPower(uint16 expectedCombatPower, uint16 minCombatPower) const
{
	// 查找最靠近expectedCombatPower的战斗力和属性阶段
	auto it = combatPowerStages.lower_bound(expectedCombatPower);
	if (it == combatPowerStages.end())
		--it;
	uint16 cpFound = (*it).first;
	uint8 stage = (*it).second;
	// 找到的战斗力不能低于minCombatPower
	while (cpFound < minCombatPower)
	{
		--it;
		if (it != combatPowerStages.end())
		{
			cpFound = (*it).first;
			stage = (*it).second;
		}
		else
			break;
	}

	return stage;
}

struct WaypointSortItem
{
	float distance;
	bool isSameDistrict;
	bool isLinkedToSourceWaypoint;
	uint32 districtCount;
	WaypointNode* node;
};

class WaypointSortPred 
{
public:
	bool operator()(WaypointSortItem const& lhs, WaypointSortItem const& rhs) const
	{
		if (lhs.isSameDistrict != rhs.isSameDistrict)
			return lhs.isSameDistrict < rhs.isSameDistrict;
		else
		{
			if (lhs.districtCount != rhs.districtCount)
				return lhs.districtCount < rhs.districtCount;
			else
			{
				if (lhs.isLinkedToSourceWaypoint != rhs.isLinkedToSourceWaypoint)
					return lhs.isLinkedToSourceWaypoint < rhs.isLinkedToSourceWaypoint;
				else
					return lhs.distance < rhs.distance;
			}
		}
	}
};

struct ExplorAreaSortItem
{
	float distance;
	bool isWithinSafeZone;
	ExplorArea area;
};

class ExplorAreaSortPred
{
public:
	bool operator()(ExplorAreaSortItem const& lhs, ExplorAreaSortItem const& rhs) const	{
		if (lhs.isWithinSafeZone != rhs.isWithinSafeZone)
			return lhs.isWithinSafeZone > rhs.isWithinSafeZone;
		else
			return lhs.distance < rhs.distance;
	}
};

Robot::Robot() :
	m_templateId(0),
	m_moveSpline(new RobotMoveSpline(this)),
	m_motionMaster(new MotionMaster(this)),
	m_staminaUpdater(new RobotStaminaUpdater(this)),
	m_hidingSpot(TileCoord::INVALID),
	m_combating(nullptr),
	m_combatState(COMBAT_STATE_CHASE),
	m_favorableEquipmentCount(0),
	m_favorableMagicBeanDiff(0),
	m_attacking(nullptr),
	m_isAttackModeDirty(false),
	m_attackTimer(0),
	m_attackLostTime(0),
	m_attackInterval(0),
	m_attackMinInterval(0),
	m_attackMaxInterval(0),
	m_attackSpeed(ATTACK_SPEED_NONE),
	m_numAttacks(0),
	m_isContinuousAttack(false),
	m_attackDirection(0),
	m_isAttackDelayed(false),
	m_isHandDownTimerEnabled(false),
	m_collecting(nullptr),
	m_collectionState(COLLECTION_STATE_COLLECT),
	m_wishManager(this),
	m_equipmentCount(0),
	m_unlockState(UNLOCK_STATE_CHASE),
	m_unitThreatManager(this),
	m_projThreatManager(this),
	m_stepCount(0),
	m_worldExplorCount(0),
	m_explorAreaSize(0),
	m_explorWidth(0),
	m_explorHeight(0),
	m_explorOrderCounter(0),
	m_sourceWaypoint(TileCoord::INVALID),
	m_goalExplorArea(ExplorArea::INVALID),
	m_currWaypointExplorArea(ExplorArea::INVALID),
	m_isInPatrol(false)
{
	m_type |= TYPEMASK_ROBOT;
	m_typeId = TypeID::TYPEID_ROBOT;
}

Robot::~Robot()
{
	m_attacking = nullptr;

	if (m_ai)
	{
		delete m_ai;
		m_ai = nullptr;
	}

	if (m_moveSpline)
	{
		delete m_moveSpline;
		m_moveSpline = nullptr;
	}

	if (m_motionMaster)
	{
		delete m_motionMaster;
		m_motionMaster = nullptr;
	}

	if (m_staminaUpdater)
	{
		delete m_staminaUpdater;
		m_staminaUpdater = nullptr;
	}
}

void Robot::attack()
{
	if (!m_attacking)
		return;

	if (this->getData()->hasMovementFlag(MOVEMENT_FLAG_HANDUP))
	{
		this->attackInternal();
	}
	else
	{
		NS_ASSERT_DEBUG(!m_staminaUpdater->isInCharge());
		NS_ASSERT_DEBUG(!m_isHandDownTimerEnabled);
		this->attackDelayed();
	}
}

bool Robot::attackStop()
{
	if (!m_attacking)
		return false;

	this->getData()->setTarget(ObjectGuid::EMPTY);

	m_attacking->removeWatcher(this);
	m_attacking->removeAttacker(this);

	this->stopNormalAttack();
	this->stopContinuousAttack();

	if (this->isAlive())
	{
		if (m_isAttackDelayed)
		{
			NS_ASSERT_DEBUG(!m_staminaUpdater->isInCharge());
			NS_ASSERT_DEBUG(this->getData()->hasMovementFlag(MOVEMENT_FLAG_HANDUP));

			this->getData()->clearMovementFlag(MOVEMENT_FLAG_HANDUP);
			this->sendMoveSync();
		}
	}
	else
	{
		this->stopHandDownTimer();
		this->getData()->clearMovementFlag(MOVEMENT_FLAG_HANDUP);
	}
	this->stopAttackDelayTimer();

	m_isAttackModeDirty = false;
	m_attacking = nullptr;

	this->clearUnitState(UNIT_STATE_ATTACKING);

	return true;
}

bool Robot::setAttackTarget(AttackableObject* target)
{
	if (target == m_attacking)
		return false;

	if (m_attacking)
	{
		m_attacking->removeWatcher(this);
		m_attacking->removeAttacker(this);
	}
	target->addAttacker(this);

	m_attacking = target;

	this->relocateAttackTarget();
	this->getData()->setTarget(target->getData()->getGuid());

	return true;
}

void Robot::relocateAttackTarget()
{
	if (!m_attacking)
		return;

	if (this->hasLineOfSight(m_attacking))
		m_attacking->addWatcher(this);
	else
		m_attacking->removeWatcher(this);

	m_isAttackModeDirty = true;
}

bool Robot::isAttackReady() const
{
	if (m_staminaUpdater->canAttack())
	{
		if (m_staminaUpdater->isInCharge())
			return true;
		else if ((m_attackSpeed != ATTACK_SPEED_NONE || m_isContinuousAttack) && !m_isAttackDelayed && m_attackTimer <= 0)
			return true;
	}

	return false;
}

void Robot::setAttackTimer(NSTime interval, NSTime delay)
{
	m_attackTimer = delay;
	m_attackInterval = interval;
	m_attackLostTime = 0;
}

void Robot::resetAttackTimer()
{
	m_attackTimer = m_attackInterval;
	//NS_LOG_DEBUG("behaviors.robot", "Robot::resetAttackTimer() attackLostTime: %d", m_attackLostTime);
}

void Robot::charge()
{
	if (m_staminaUpdater->isInCharge())
		return;

	this->stopNormalAttack();
	this->stopContinuousAttack();
	this->stopAttackDelayTimer();

	if (m_isHandDownTimerEnabled)
		this->stopHandDownTimer();
	else if (!this->getData()->hasMovementFlag(MOVEMENT_FLAG_HANDUP))
	{
		this->getData()->addMovementFlag(MOVEMENT_FLAG_HANDUP);
		this->sendMoveSync();
	}
	NS_ASSERT_DEBUG(this->getData()->hasMovementFlag(MOVEMENT_FLAG_HANDUP));

	m_staminaUpdater->charge();
}

void Robot::chargeStop()
{
	if (!m_staminaUpdater->isInCharge())
		return;

	if (this->getData()->hasMovementFlag(MOVEMENT_FLAG_HANDUP))
	{
		this->getData()->clearMovementFlag(MOVEMENT_FLAG_HANDUP);
		this->sendMoveSync();
	}

	m_staminaUpdater->chargeStop();
}

bool Robot::canCharge() const
{
	if (!this->hasItemEffectType(ITEM_EFFECT_CHARGED_ATTACK_ENABLED))
		return false;

	if (m_staminaUpdater->isInCharge())
		return false;

	if (m_attacking && !this->hasUnitState(UNIT_STATE_MOVING))
		return false;

	float staminaRatio = this->getData()->getStamina() / (float)this->getData()->getMaxStamina();
	if (staminaRatio < CHARGE_START_STAMINA_RATIO)
		return false;

	return true;
}

void Robot::startNormalAttack(AttackSpeed speed)
{
	if (m_attackSpeed == speed)
		return;

	m_attackSpeed = speed;
	this->stopContinuousAttack();

	switch (speed)
	{
	case ATTACK_SPEED_SLOW:
	{
		int32 maxStamina = this->getData()->getMaxStamina();
		float points = this->getData()->getStaminaRegenRate() * maxStamina;
		NSTime attackInr = (NSTime)std::ceil(this->getData()->getAttackTakesStamina() / points * 1000);
		attackInr = std::max(NORMAL_ATTACK_MIN_INTERVAL, attackInr);
		m_attackMinInterval = attackInr;
		m_attackMaxInterval = attackInr * 2;
		break;
	}
	case ATTACK_SPEED_FAST:
		m_attackMinInterval = NORMAL_ATTACK_MIN_INTERVAL;
		m_attackMaxInterval = NORMAL_ATTACK_MAX_INTERVAL;
		break;
	}

	NSTime delay = m_attackMinInterval;
	this->setAttackTimer(0, delay);

	//NS_LOG_DEBUG("behaviors.robot", "Robot::startNormalAttack() speed: %d delay: %d minInterval: %d maxInterval: %d", speed, delay, m_attackMinInterval, m_attackMaxInterval);
}

void Robot::stopNormalAttack()
{
	if (m_attackSpeed == ATTACK_SPEED_NONE)
		return;

	m_attackSpeed = ATTACK_SPEED_NONE;
	m_attackMinInterval = 0;
	m_attackMaxInterval = 0;

	this->resetAttackTimer();
	//NS_LOG_DEBUG("behaviors.robot", "Robot::stopNormalAttack()");
}

bool Robot::startContinuousAttack()
{
	if (m_isContinuousAttack)
		return true;

	this->stopNormalAttack();

	// 首次攻击的延迟，模拟玩家瞄准花费的时间
	RobotProficiency const* proficiency = sObjectMgr->getRobotProficiency(this->getData()->getProficiencyLevel());
	NS_ASSERT(proficiency);
	int32 delay = random(proficiency->minContinuousAttackDelay, proficiency->maxContinuousAttackDelay);

	float regenSpeed = this->getData()->getStaminaRegenRate() * this->getData()->getMaxStamina();
	int32 regenPoints = (int32)(regenSpeed * (delay / 1000.f)) + this->getData()->getStamina();
	regenPoints = std::min(this->getData()->getMaxStamina(), regenPoints);
	int32 nAttacks = regenPoints / this->getData()->getAttackTakesStamina();

	if (nAttacks > 0)
	{
		int32 regenPoints = (int32)(regenSpeed * ((nAttacks - 1) * (ATTACK_INTERVAL / 1000.f)));
		nAttacks += regenPoints / this->getData()->getAttackTakesStamina();
		if (nAttacks >= EXPECTED_NUMBER_OF_ATTACKS_MIN)
		{
			m_numAttacks = nAttacks;
			m_isContinuousAttack = true;
			m_attackDirection = FLT_MAX;

			this->setAttackTimer(ATTACK_INTERVAL, delay);

			//NS_LOG_DEBUG("behaviors.robot", "Robot::startContinuousAttack() stamina: %d isInfiniteStamina: %d numAttacks: %d delay: %d", this->getData()->getStamina(), this->getData()->isInfiniteStamina(), m_numAttacks, delay);
			return true;

		}
	}

	return false;
}

void Robot::stopContinuousAttack()
{
	if (!m_isContinuousAttack)
		return;

	m_isContinuousAttack = false;
	m_numAttacks = 0;
	m_attackDirection = FLT_MAX;

	this->resetAttackTimer();

	//NS_LOG_DEBUG("behaviors.robot", "Robot::stopContinuousAttack()");
}

float Robot::calcAimingDirection(AttackableObject* target) const
{
	Point aimPoint = target->getData()->getPosition();
	float error = 0.f;
	if (Unit* unit = target->asUnit())
	{
		if (unit->hasUnitState(UNIT_STATE_MOVING)
			|| (unit->isType(TYPEMASK_PLAYER) && unit->asPlayer()->getData()->isTrainee()))
		{
			RobotProficiency const* proficiency = sObjectMgr->getRobotProficiency(this->getData()->getProficiencyLevel());
			NS_ASSERT(proficiency);
			float max = AIMING_ERROR_RANGE_MAX * (1.0f - proficiency->aimingAccuracy);
			if (max > 0.f)
				error = random(0.f, max);
		}
	}

	if (error > 0.f)
	{
		Point tp1, tp2;
		bool ret = MathTools::findTangentPointsInCircle(this->getData()->getPosition(), aimPoint, error, tp1, tp2);
		if (ret)
			aimPoint = random(0, 1) == 1 ? tp1 : tp2;
	}

	float dir = MathTools::computeAngleInRadians(this->getData()->getPosition(), aimPoint);
	return dir;
}

float Robot::applyThreatModifier(UnitThreatType type, float threat)
{
	return threat * m_unitThreatModifiers[m_combatState][type];
}

Unit* Robot::selectVictim()
{
	Unit* victim = m_unitThreatManager.getHostileTarget();
	return victim;
}

float Robot::calcThreat(Unit* victim, UnitThreatType type, float variant)
{
	float threat = 0.f;

	switch (type)
	{
	case UNITTHREAT_DISTANCE:
	{
		float dist = this->getData()->getPosition().getDistance(victim->getData()->getPosition());
		threat = 1.0f - std::min(1.0f, dist / ENEMY_CRITICAL_THREAT_DISTANCE);
		break;
	}
	case UNITTHREAT_ENEMY_HEALTH:
		threat = 1.0f - victim->getData()->getHealth() / (float)victim->getData()->getMaxHealth();
		break;
	case UNITTHREAT_RECEIVED_DAMAGE:
		threat = std::min(1.0f, variant / this->getData()->getMaxHealth());
		break;
	case UNITTHREAT_ENEMY_CHARGED_POWER:
		threat = victim->getData()->getChargeStartStamina() / (float)victim->getData()->getMaxStamina();
		break;
	}

	return threat;
}

void Robot::deleteUnitThreatList()
{
	m_unitThreatManager.removeAllThreats();
}

void Robot::addThreat(Unit* victim, UnitThreatType type, float variant)
{
	m_unitThreatManager.addThreat(victim, type, variant);
}

bool Robot::isWithinCriticalThreatRange(Unit* victim) const
{
	return this->isWithinDist(victim, ENEMY_CRITICAL_THREAT_DISTANCE);
}

bool Robot::isHostileTarget(Unit* victim) const
{
	return m_unitThreatManager.isHostileTarget(victim);
}

bool Robot::isPotentialThreat(Unit* victim) const
{
	if (this->canActiveAttack(victim))
		return true;

	if (victim->hasUnitState(UNIT_STATE_CHARGING))
		return true;

	if (this->isWithinCriticalThreatRange(victim))
		return true;

	return false;
}

Projectile* Robot::selectProjectileToDodge()
{
	Projectile* proj = m_projThreatManager.getHostileTarget();

	return proj;
}

void Robot::deleteProjectileThreatList()
{
	m_projThreatManager.removeAllThreats();
}

void Robot::addThreat(Projectile* proj)
{
	m_projThreatManager.addThreat(proj);
}

bool Robot::isHostileTarget(Projectile* proj) const
{
	return m_projThreatManager.isHostileTarget(proj);
}

bool Robot::isPotentialThreat(Projectile* proj) const
{
	if (!this->isAlive())
		return false;

	if (!proj->isInWorld() || !proj->canDetect(const_cast<Robot*>(this)) || !proj->isInProjectedPath(const_cast<Robot*>(this)))
		return false;

	return true;
}

bool Robot::loadData(SimpleRobot const& simpleRobot, BattleMap* map)
{
	DataRobot* data = new DataRobot();
	data->setGuid(ObjectGuid(GUIDTYPE_ROBOT, map->generateRobotSpawnId()));
	this->setData(data);

	if (this->getLocator())
	{
		DataUnitLocator* dataLocator = new DataUnitLocator();
		dataLocator->setGuid(ObjectGuid(GUIDTYPE_UNIT_LOCATOR, map->generateUnitLocatorSpawnId()));
		this->getLocator()->setData(dataLocator);
	}

	return this->reloadData(simpleRobot, map);
}

bool Robot::reloadData(SimpleRobot const& simpleRobot, BattleMap* map)
{
	DataRobot* data = this->getData();
	if (!data)
		return false;

	m_templateId = simpleRobot.templateId;

	RobotTemplate const* tmpl = sObjectMgr->getRobotTemplate(m_templateId);
	NS_ASSERT(tmpl);

	data->setObjectSize(UNIT_OBJECT_SIZE);
	data->setAnchorPoint(UNIT_ANCHOR_POINT);
	data->setObjectRadiusInMap(UNIT_OBJECT_RADIUS_IN_MAP);
	data->setLaunchCenter(UNIT_LAUNCH_CENTER);
	data->setLaunchRadiusInMap(UNIT_LAUNCH_RADIUS_IN_MAP);

	data->setDisplayId(tmpl->displayId);
	data->setName(simpleRobot.name);
	data->setCountry(simpleRobot.country);
	data->setExperience(0);
	data->setLevel(simpleRobot.level);
	if (simpleRobot.level < LEVEL_MAX)
		data->setNextLevelXP(assertNotNull(sObjectMgr->getUnitLevelXP(simpleRobot.level + 1))->experience);
	else
		data->setNextLevelXP(0);
	data->setStage(simpleRobot.stage);
	data->setCombatPower(tmpl->getStageStats(simpleRobot.stage).combatPower);

	auto maxHealth = tmpl->getStageStats(simpleRobot.stage).maxHealth;
	data->setHealth(maxHealth);
	data->setMaxHealth(maxHealth);
	data->setBaseMaxHealth(maxHealth);
	auto healthRegenRate = tmpl->getStageStats(simpleRobot.stage).healthRegenRate;
	data->setHealthRegenRate(healthRegenRate);
	data->setBaseHealthRegenRate(healthRegenRate);
	auto attackRange = tmpl->getStageStats(simpleRobot.stage).attackRange;
	data->setAttackRange(attackRange);
	data->setBaseAttackRange(attackRange);
	auto moveSpeed = tmpl->getStageStats(simpleRobot.stage).moveSpeed;
	data->setMoveSpeed(moveSpeed);
	data->setBaseMoveSpeed(moveSpeed);
	auto maxStamina = tmpl->getStageStats(simpleRobot.stage).maxStamina;
	data->setStamina(maxStamina);
	data->setMaxStamina(maxStamina);
	data->setBaseMaxStamina(maxStamina);
	auto staminaRegenRate = tmpl->getStageStats(simpleRobot.stage).staminaRegenRate;
	data->setStaminaRegenRate(staminaRegenRate);
	data->setBaseStaminaRegenRate(staminaRegenRate);
	auto attackTakesStamina = tmpl->getStageStats(simpleRobot.stage).attackTakesStamina;
	data->setAttackTakesStamina(attackTakesStamina);
	data->setBaseAttackTakesStamina(attackTakesStamina);
	auto chargeConsumesStamina = tmpl->getStageStats(simpleRobot.stage).chargeConsumesStamina;
	data->setChargeConsumesStamina(chargeConsumesStamina);
	data->setBaseChargeConsumesStamina(chargeConsumesStamina);
	auto damage = tmpl->getStageStats(simpleRobot.stage).damage;
	data->setDamage(damage);
	data->setBaseDamage(damage);
	data->setDefense(0);

	data->setReactFlags(simpleRobot.reactFlags);
	data->setProficiencyLevel(simpleRobot.proficiencyLevel);
	data->setNatureType(simpleRobot.natureType);
	data->setUnitFlags(UNIT_FLAG_NONE);
	data->setSmiley(SMILEY_NONE);
	data->setConcealmentState(CONCEALMENT_STATE_EXPOSED);
	data->setPickupDuration(ITEM_PICKUP_DURATION);
	data->setMagicBeanCount(0);

	TileCoord spawnPoint;
	if (simpleRobot.spawnPoint.isInvalid())
		spawnPoint = map->generateUnitSpawnPoint();
	else
		spawnPoint = simpleRobot.spawnPoint;
	data->setSpawnPoint(spawnPoint);
	Point position = data->getSpawnPoint().computePosition(map->getMapData()->getMapSize());
	data->setPosition(position);
	data->randomOrientation();

	if (this->getLocator())
	{
		DataUnitLocator* dataLocator = this->getLocator()->getData();
		dataLocator->setDisplayId(tmpl->displayId);
		dataLocator->setMoveSpeed(data->getMoveSpeed());
		dataLocator->setPosition(position);
		dataLocator->setAlive(true);
	}

	m_deathState = DEATH_STATE_ALIVE;
	m_rankNo = 0;
	m_battleOutcome = BATTLE_VICTORY;
	m_withdrawalState = WITHDRAWAL_STATE_NONE;
	m_dangerState = DANGER_STATE_RELEASED;
	m_attackMinInterval = 0;
	m_attackMaxInterval = 0;
	m_equipmentCount = 0;

	m_explorAreaSize = 0;
	m_explorWidth = 0;
	m_explorHeight = 0;
	m_explorAreaOffset = Point::ZERO;

	NS_ASSERT(m_unitState == UNIT_STATE_NONE);

	this->resetCarriedItemSpawnIdCounter();
	this->resetAllStatModifiers();
	this->resetConcealmentTimer();
	this->resetDangerStateTimer();
	this->resetWithdrawalTimer();
	this->resetHealthRegenTimer();
	this->resetHealthLossTimer();
	this->resetUnsaySmileyTimer();
	this->setUnlockState(UNLOCK_STATE_CHASE);
	this->setCollectionState(COLLECTION_STATE_COLLECT);

	RobotNature const* nature = sObjectMgr->getRobotNature(simpleRobot.natureType);
	m_unitThreatModifiers[COMBAT_STATE_CHASE][UNITTHREAT_DISTANCE] = nature->getEffectValue(NATURE_EFFECT_CHASE_THREAT_DISTANCE_WEIGHTED_PERCENT) / 100.f;
	m_unitThreatModifiers[COMBAT_STATE_CHASE][UNITTHREAT_ENEMY_HEALTH] = nature->getEffectValue(NATURE_EFFECT_CHASE_THREAT_ENEMY_HEALTH_WEIGHTED_PERCENT) / 100.f;
	m_unitThreatModifiers[COMBAT_STATE_CHASE][UNITTHREAT_RECEIVED_DAMAGE] = nature->getEffectValue(NATURE_EFFECT_CHASE_THREAT_RECEIVED_DAMAGE_WEIGHTED_PERCENT) / 100.f;
	m_unitThreatModifiers[COMBAT_STATE_CHASE][UNITTHREAT_ENEMY_CHARGED_POWER] = nature->getEffectValue(NATURE_EFFECT_CHASE_THREAT_ENEMY_CHARGED_POWER_WEIGHTED_PERCENT) / 100.f;

	m_unitThreatModifiers[COMBAT_STATE_ESCAPE][UNITTHREAT_DISTANCE] = 0.9f;
	m_unitThreatModifiers[COMBAT_STATE_ESCAPE][UNITTHREAT_ENEMY_HEALTH] = 0.f;
	m_unitThreatModifiers[COMBAT_STATE_ESCAPE][UNITTHREAT_RECEIVED_DAMAGE] = 0.05f;
	m_unitThreatModifiers[COMBAT_STATE_ESCAPE][UNITTHREAT_ENEMY_CHARGED_POWER] = 0.05f;

	if (map->isTrainingGround())
	{
		this->setCombatState(COMBAT_STATE_CHASE);
		this->initializeAI(ROBOTAI_TYPE_TRAINING);
		this->getMotionMaster()->moveIdle();
	}
	else
	{
		this->setCombatState(COMBAT_STATE_ESCAPE);
		this->initializeAI(ROBOTAI_TYPE_SPARRING);
		this->setupExplorAreas(map->getMapData(), data->getPosition());
		(static_cast<SparringRobotAI*>(this->getAI()))->exploreDelayed();
	}

	return true;
}


void Robot::addToWorld()
{
	if (this->isInWorld())
		return;

	Unit::addToWorld();

	ObjectAccessor::addObject(this);

	m_map->increaseAliveCounter();
	this->concealIfNeeded(this->getData()->getPosition());
}

void Robot::removeFromWorld()
{
	if (!this->isInWorld())
		return;

	this->stopMoving();
	this->hideStop();
	this->exploreStop();
	this->collectStop();

	if (this->isAlive())
	{
		m_map->decreaseAliveCounter();

		this->stopAttackDelayTimer();
		this->stopHandDownTimer();
		this->getData()->clearMovementFlag(MOVEMENT_FLAG_HANDUP);
	}

	ObjectAccessor::removeObject(this);

	Unit::removeFromWorld();
}

void Robot::update(NSTime diff)
{
	Unit::update(diff);

	if (!this->isInWorld())
		return;

	if (this->isAlive())
	{
		m_moveSpline->update(diff);
		m_projThreatManager.update();
		m_motionMaster->updateMotion(diff);
		m_staminaUpdater->update(diff);

		this->updateAttackTimer(diff);
		this->updateAttackDelayTimer(diff);

		if(this->getAI())
			this->getAI()->updateAI(diff);
		this->updateAttackMode();
		this->updateHandDownTimer(diff);
	}
	this->updateWithdrawalTimer(diff);
}

void Robot::cleanupBeforeDelete()
{
	if (this->isInWorld())
		this->removeFromWorld();

	this->deleteUnitThreatList();
	this->deleteProjectileThreatList();
	this->deleteWishList();
	if(this->getAI())
		this->getAI()->resetAI();
	this->clearCheckedHidingSpots();
	this->cleanupExplorAreas();

	Unit::cleanupBeforeDelete();
}

void Robot::setDeathState(DeathState state)
{
	Unit::setDeathState(state);

	switch (state)
	{
	case DEATH_STATE_ALIVE:
		break;
	case DEATH_STATE_DEAD:
		this->hideStop();
		this->exploreStop();
		this->collectStop();

		this->deleteUnitThreatList();
		this->deleteProjectileThreatList();
		this->deleteWishList();
		if (this->getAI())
			this->getAI()->resetAI();
		this->clearCheckedHidingSpots();
		this->cleanupExplorAreas();
		break;
	}
}

void Robot::died(Unit* killer)
{
	Unit* champion;
	m_rewardManager.awardAllAwardees(&champion);

	std::vector<LootItem> itemList;
	this->fillLoot(killer, itemList);
	if (!itemList.empty())
		this->dropLoot(killer, itemList, nullptr);

	std::stringstream itemsStr;
	for (int32 i = 0; i < UNIT_SLOTS_COUNT; ++i)
	{
		CarriedItem* item = this->getItem(i);
		if (item)
			itemsStr << item->getData()->getItemId() << " ";
	}
	NS_LOG_DEBUG("behaviors.robot", "Robot (guid: 0x%08X) died Proficiency: %d NatureType: %d MagicBeanCount: %d EquipmentCount: %d Items: %s", this->getData()->getGuid().getRawValue(), this->getData()->getProficiencyLevel(), this->getData()->getNatureType(), this->getData()->getMagicBeanCount(), m_equipmentCount, itemsStr.str().c_str());

	if(this->getLocator())
		this->getLocator()->getData()->setAlive(false);

	this->setDeathState(DEATH_STATE_DEAD);
	m_map->robotKilled(this);

	if(!m_map->isTrainingGround())
		this->sendDeathMessageToAllPlayers(killer);
}

void Robot::kill(Unit* victim)
{
	victim->died(this);

	int32 killCount = this->getData()->getKillCount();
	this->getData()->setKillCount(killCount + 1);
}

void Robot::kill(ItemBox* itemBox)
{
	itemBox->opened(this);
}

void Robot::giveMoney(int32 money)
{
	if (money <= 0)
		return;

	this->modifyMoney(money);
}

void Robot::giveXP(int32 xp)
{
	this->addExperience(xp);
}

void Robot::withdraw(BattleOutcome outcome, int32 rankNo)
{
	m_map->addObjectToRemoveList(this);
}

bool Robot::canHide(TileCoord const& hidingSpot) const
{
	if (!this->isAlive())
		return false;

	if (m_map->isBattleEnding())
		return false;

	// 如果正在躲藏则需要等待生命值完全恢复
	if (this->hasUnitState(UNIT_STATE_HIDING))
	{
		if (this->getData()->getHealth() >= this->getData()->getMaxHealth())
			return false;
	}
	else
	{
		RobotNature const* nature = sObjectMgr->getRobotNature(this->getData()->getNatureType());
		float healthPercent = this->getData()->getHealth() / (float)this->getData()->getMaxHealth() * 100;
		int32 abandonHidingHealthPercent = nature->getEffectValue(NATURE_EFFECT_ABANDON_HIDING_HEALTH_PERCENT);
		if (healthPercent > abandonHidingHealthPercent)
			return false;
		// 有急救物品的情况下
		if (this->getItem(INVENTORY_SLOT_FIRST_AID) && healthPercent > abandonHidingHealthPercent * 0.75f)
			return false;
	}

	// 有对自己构成威胁的人
	if (!m_unitThreatManager.getThreatList().empty())
		return false;

	// 躲藏点没有与危险区保持安全距离
	if (!m_map->isSafeDistanceMaintained(hidingSpot))
		return false;

	// 躲藏点在危险地区
	if (m_map->isDangerousDistrict(hidingSpot))
		return false;

	return true;
}

bool Robot::setHidingSpot(TileCoord const& tileCoord)
{
	NS_ASSERT(this->canHide(tileCoord));

	if (tileCoord == m_hidingSpot)
		return false;

	this->addUnitState(UNIT_STATE_HIDING);

	m_hidingSpot = tileCoord;

	return true;
}

void Robot::hideStop()
{
	if (m_hidingSpot.isInvalid())
		return;

	m_hidingSpot = TileCoord::INVALID;
	this->clearUnitState(UNIT_STATE_HIDING);
	if (this->getAI())
		this->getAI()->clearAIAction(AI_ACTION_TYPE_HIDE);
}

bool Robot::canSeek(TileCoord const& hidingSpot) const
{
	if (!this->isAlive())
		return false;

	if (m_map->isBattleEnding())
		return false;

	if (!this->hasWorldExplored())
		return false;

	if (!m_map->isSafeDistanceMaintained(hidingSpot))
		return false;

	if (this->isHidingSpotChecked(hidingSpot))
		return false;

	return true;
}

bool Robot::isHidingSpotChecked(TileCoord const& tileCoord) const
{
	int32 tileIndex = m_map->getMapData()->getTileIndex(tileCoord);
	return m_checkedHidingSpotLookupTable.find(tileIndex) != m_checkedHidingSpotLookupTable.end();
}

void Robot::markHidingSpotChecked(TileCoord const& tileCoord)
{
	if (this->isHidingSpotChecked(tileCoord))
		return;

	int32 tileIndex = m_map->getMapData()->getTileIndex(tileCoord);
	NSTime time = (NSTime)(getUptimeMillis() / (int64)1000);
	auto it = m_checkedHidingSpotList.emplace(m_checkedHidingSpotList.end(), std::make_pair(tileCoord, time));
	NS_ASSERT(it != m_checkedHidingSpotList.end());
	auto ret = m_checkedHidingSpotLookupTable.emplace(tileIndex, it);
	NS_ASSERT(ret.second);
}

void Robot::clearCheckedHidingSpots()
{
	m_checkedHidingSpotList.clear();
	m_checkedHidingSpotLookupTable.clear();
}

bool Robot::canCollect(Item* supply) const
{
	if (!this->canPickup(supply))
		return false;

	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(supply->getData()->getItemId());
	NS_ASSERT(tmpl);

	// 战斗结束中只收集金币
	if (m_map->isBattleEnding() && tmpl->itemClass != ITEM_CLASS_GOLD)
		return false;

	// 如果装备了0级雪球夹则放弃收集其他雪球夹
	if (tmpl->itemClass == ITEM_CLASS_EQUIPMENT && tmpl->itemSubClass == ITEM_SUBCLASS_SNOWBALL_MAKER)
	{
		CarriedItem* item = this->getItem(EQUIPMENT_SLOT_SNOWBALL_MAKER);
		if (item && item->getData()->getLevel() == 0)
			return false;
	}

	// 放弃距离正在收集的物品较远的物品
	if (m_collecting)
	{
		float dist = m_collecting->getData()->getPosition().getDistance(supply->getData()->getPosition());
		if (dist > ITEMS_TO_COLLECT_AND_TARGETED_ITEM_DISTANCE)
			return false;
	}

	if (!this->canCollectInCombat())
		return false;

	// 物品正在被其他机器人收集
	if(supply->getCollectorCount() > 0 && (supply->getCollectorCount() > 1 || !supply->isCollector(const_cast<Robot*>(this))))
		return false;

	// 物品正在被其他单位捡拾
	if(supply->getPickerCount() > 0 && (supply->getPickerCount() > 1 || !supply->isPicker(const_cast<Robot*>(this))))
		return false;

	// 物品在危险区中
	TileCoord itemCoord(m_map->getMapData()->getMapSize(), supply->getData()->getPosition());
	if(!m_map->isWithinSafeZone(itemCoord))
		return false;

	return true;
}

bool Robot::setCollectTarget(Item* supply)
{
	NS_ASSERT(this->canCollect(supply));

	if (supply == m_collecting)
		return false;

	this->addUnitState(UNIT_STATE_IN_COLLECTION);

	if (m_collecting)
		m_collecting->removeCollector(this);
	supply->addCollector(this);

	m_collecting = supply;

	this->addWish(supply);

	return true;
}

bool Robot::removeCollectTarget()
{
	if (!m_collecting)
		return false;

	m_collecting->removeCollector(this);
	m_collecting = nullptr;

	return true;
}

void Robot::collectStop()
{
	this->removeCollectTarget();

	this->setCollectionState(COLLECTION_STATE_COLLECT);
	this->clearUnitState(UNIT_STATE_IN_COLLECTION);

	if (this->getAI())
	{
		this->getAI()->clearAIAction(AI_ACTION_TYPE_COLLECT);
		this->getAI()->clearAIAction(AI_ACTION_TYPE_COLLECT_DIRECTLY);
	}
}

bool Robot::checkCollectionCollectConditions() const
{
	NS_ASSERT(m_collecting);

	if (!this->isWithinDist(m_collecting, this->getSightDistance()))
		return true;

	if(!this->isInCombat())
		return true;

	return false;
}

bool Robot::checkCollectionCombatConditions() const
{
	NS_ASSERT(m_collecting);

	// 正在捡拾收集的目标
	if (m_collecting != this->getPickupTarget())
		return false;

	RobotNature const* nature = sObjectMgr->getRobotNature(this->getData()->getNatureType());
	int32 lockItemPickupTimeRemaining = nature->getEffectValue(NATURE_EFFECT_LOCK_ITEM_PICKUP_TIME_REMAINING);
	lockItemPickupTimeRemaining = std::min(lockItemPickupTimeRemaining, ITEM_PICKUP_DURATION);
	if (m_pickupTimer.getRemainder() < lockItemPickupTimeRemaining)
		return false;

	if (m_projThreatManager.getThreatList().empty())
		return false;

	if (!this->isInCombat())
		return false;

	return true;
}

bool Robot::updateCollectionState()
{
	if (!this->isInCollection())
		return false;

	switch (m_collectionState)
	{
	case COLLECTION_STATE_COLLECT:
		if (this->checkCollectionCombatConditions())
		{
			m_collectionState = COLLECTION_STATE_COMBAT;
			return true;
		}
		break;
	case COLLECTION_STATE_COMBAT:
		if (this->checkCollectionCollectConditions())
		{
			m_collectionState = COLLECTION_STATE_COLLECT;
			return true;
		}
		break;
	default:
		break;
	}

	return false;
}

void Robot::addWish(Item* supply)
{
	m_wishManager.addWish(supply);
}

Item* Robot::selectWish()
{
	Item* item = m_wishManager.getInterestedTarget();
	return item;
}

void Robot::deleteWishList()
{
	m_wishManager.removeAllWishes();
}

bool Robot::isInterestedTarget(Item* item) const
{
	return m_wishManager.isInterestedTarget(item);
}

bool Robot::canPickup(Item* supply) const
{
	if (!Unit::canPickup(supply))
		return false;

	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(supply->getData()->getItemId());
	NS_ASSERT(tmpl);
	PickupStatus status = this->canStoreItem(tmpl, supply->getData()->getCount(), nullptr);
	if (status != PICKUP_STATUS_OK)
		return false;

	return true;
}

bool Robot::pickup(Item* supply)
{
	if(!Unit::pickup(supply))
		return false;

	return true;
}

bool Robot::pickupStop()
{
	if (!Unit::pickupStop())
		return false;

	return true;
}

void Robot::receiveItem(Item* supply)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(supply->getData()->getItemId());
	NS_ASSERT(tmpl);

	ItemDest dest;
	if (this->canStoreItem(tmpl, supply->getData()->getCount(), &dest) != PICKUP_STATUS_OK)
		return;

	supply->receiveBy(this);
	this->storeItem(tmpl, dest);
}

void Robot::storeItem(ItemTemplate const* itemTemplate, ItemDest const& dest)
{
	Unit::storeItem(itemTemplate, dest);
	if (itemTemplate->itemClass == ITEM_CLASS_MAGIC_BEAN)
		this->updateFavorableMagicBeanDiff();
	else if(itemTemplate->itemClass == ITEM_CLASS_EQUIPMENT)
		this->updateFavorableEquipmentCount();
}

void Robot::removeItem(CarriedItem* item)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item->getData()->getItemId());
	NS_ASSERT(tmpl);
	if (tmpl->itemClass == ITEM_CLASS_EQUIPMENT)
		--m_equipmentCount;

	Unit::removeItem(item);
}

CarriedItem* Robot::createItem(ItemTemplate const* itemTemplate, ItemDest const& dest)
{
	CarriedItem* item = Unit::createItem(itemTemplate, dest);
	if(itemTemplate->itemClass == ITEM_CLASS_EQUIPMENT)
		++m_equipmentCount;

	return item;
}

bool Robot::canUseItem(CarriedItem* item) const
{
	if (!Unit::canUseItem(item))
		return false;

	if (m_map->isBattleEnding())
		return false;

	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item->getData()->getItemId());
	NS_ASSERT(tmpl);
	ItemApplicationTemplate const* appTmpl = sObjectMgr->getItemApplicationTemplate(tmpl->appId);
	if (!appTmpl)
		return false;

	bool cond = false;
	for (auto const& effect : appTmpl->effects)
	{
		switch (effect.type)
		{
		case ITEM_EFFECT_APPLY_STATS:
		case ITEM_EFFECT_DAMAGE_BONUS_PERCENT:
			cond |= this->checkUseItemChaseConditions(item);
			break;
		case ITEM_EFFECT_DISCOVER_CONCEALED_UNIT:
			cond |= this->checkUseItemFindEnemiesConditions(item);
			break;
		case ITEM_EFFECT_INCREASE_HEALTH_PERCENT:
		case ITEM_EFFECT_INCREASE_HEALTH:
			cond |= this->checkUseItemFirstAidConditions(item);
			break;
		case ITEM_EFFECT_PREVENT_MOVE_SPEED_SLOWED:
		case ITEM_EFFECT_DAMAGE_REDUCTION_PERCENT:
			cond |= this->checkUseItemEscapeConditions(item);
			break;
		default:
			break;
		}
		if (cond)
			break;
	}

	return cond;
}

void Robot::useItem(CarriedItem* item)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item->getData()->getItemId());
	NS_ASSERT(tmpl);

	m_itemCooldownProcesser->startCooldown(item);
	m_itemApplicationProcesser->applyItem(tmpl);
	this->takeItem(item);
}

void Robot::unregisterItemEffect(ItemEffect const* effect)
{
	if (effect->type == ITEM_EFFECT_CHARGED_ATTACK_ENABLED)
	{
		if (m_staminaUpdater->isInCharge())
		{
			this->chargeStop();
			m_isAttackModeDirty = true;
		}
	}

	Unit::unregisterItemEffect(effect);
}

bool Robot::canUnlock(ItemBox* itemBox) const
{
	if (!Unit::canUnlock(itemBox))
		return false;

	if (m_map->isBattleEnding())
		return false;

	if (!this->canUnlockInCombat())
		return false;

	// 物品箱正在被其他单位解锁
	if (itemBox->getUnlockerCount() > 0 && (itemBox->getUnlockerCount() > 1 || !itemBox->isUnlocker(const_cast<Robot*>(this))))
		return false;

	if (this->hasUnitState(UNIT_STATE_HIDING))
		return false;

	// 物品箱在危险区中
	TileCoord itemBoxCoord(m_map->getMapData()->getMapSize(), itemBox->getData()->getPosition());
	if (!m_map->isWithinSafeZone(itemBoxCoord))
		return false;

	float dist = std::max((float)ABANDON_ATTACK_DISTANCE, this->getSightDistance());
	if (!this->isWithinDist(itemBox, dist))
		return false;

	return true;
}

void Robot::unlockStop()
{
	this->setUnlockState(UNLOCK_STATE_CHASE);
	if (this->getAI())
	{
		this->getAI()->clearAIAction(AI_ACTION_TYPE_UNLOCK);
		this->getAI()->clearAIAction(AI_ACTION_TYPE_UNLOCK_DIRECTLY);
	}

	this->attackStop();
	this->chargeStop();

	Unit::unlockStop();
}

bool Robot::checkUnlockChaseConditions() const
{
	NS_ASSERT(m_unlocking);

	if (this->isWithinDist(m_unlocking, ATTACK_FAST_WITHIN_DIST))
		return true;

	return false;
}

bool Robot::checkUnlockGoBackConditions() const
{
	NS_ASSERT(m_unlocking);

	if (!this->isWithinDist(m_unlocking, this->getSightDistance()))
		return true;

	return false;
}

bool Robot::updateUnlockState()
{
	if (!this->isInUnlock())
		return false;

	switch (m_unlockState)
	{
	case UNLOCK_STATE_CHASE:
		if (this->checkUnlockGoBackConditions())
		{
			m_unlockState = UNLOCK_STATE_GO_BACK;
			return true;
		}
		break;
	case UNLOCK_STATE_GO_BACK:
		if (this->checkUnlockChaseConditions())
		{
			m_unlockState = UNLOCK_STATE_CHASE;
			return true;
		}
		break;
	default:
		break;
	}

	return false;
}

bool Robot::canExploreArea() const
{
	return true;
}

void Robot::exploreStop()
{
	this->clearUnitState(UNIT_STATE_IN_EXPLORATION);
	if (this->getAI())
		this->getAI()->clearAIAction(AI_ACTION_TYPE_EXPLORE);
}

void Robot::stopMoving()
{
	this->getMotionMaster()->clear();

	Unit::stopMoving();
}

void Robot::setAI(RobotAIType type)
{
	if (this->getAI())
	{
		this->deleteUnitThreatList();
		this->deleteProjectileThreatList();
		this->deleteWishList();
		this->combatStop();
		this->hideStop();
		this->pickupStop();
		this->collectStop();
		this->unlockStop();
		this->exploreStop();
		this->stopMoving();
		this->clearCheckedHidingSpots();
		this->cleanupExplorAreas();
	}
	this->initializeAI(type);
}

bool Robot::updatePosition(Point const& newPosition)
{
	bool relocated = Unit::updatePosition(newPosition);
	if (relocated)
	{
		MapData* mapData = m_map->getMapData();
		TileCoord oldCoord(mapData->getMapSize(), this->getData()->getPosition());
		TileCoord newCoord(mapData->getMapSize(), newPosition);

		m_map->robotRelocation(this, newPosition);
		if(this->getLocator())
			this->getLocator()->getData()->setPosition(newPosition);
		this->relocationTellAttackers();
		this->relocateAttackTarget();

		if (oldCoord != newCoord)
		{
			this->relocateHidingSpot(newCoord);
			this->updateConcealmentState(newCoord);
		}
	}

	return relocated;
}

void Robot::transport(Point const& dest)
{
	this->deleteUnitThreatList();
	this->deleteProjectileThreatList();
	this->deleteWishList();
	this->combatStop();
	this->hideStop();
	this->pickupStop();
	this->collectStop();
	this->unlockStop();
	this->exploreStop();
	this->stopMoving();
	this->clearCheckedHidingSpots();
	this->cleanupExplorAreas();

	this->invisibleForNearbyPlayers();

	if (this->getAI())
	{
		this->getAI()->resetAI();
		if(this->getAI()->getType() == ROBOTAI_TYPE_SPARRING)
			(static_cast<SparringRobotAI*>(this->getAI()))->exploreDelayed();
	}

	if (this->isAlive())
	{
		this->expose();
		this->concealIfNeeded(dest);
	}

	// 设置新的位置
	this->getMap()->robotRelocation(this, dest);

	if (this->getLocator())
	{
		this->getLocator()->invisibleForNearbyPlayers();
		this->getLocator()->getData()->setPosition(dest);
	}

	this->updateObjectVisibility();
	this->updateObjectTraceability();
	this->updateObjectSafety();
}

void Robot::setFacingToObject(WorldObject* object)
{
	float angle = MathTools::computeAngleInRadians(this->getData()->getPosition(), object->getData()->getPosition());
	// NS_LOG_DEBUG("behaviors.robot", "Attack facing angle:%f", angle);
	getData()->setOrientation(angle);
}

bool Robot::canCombatWith(Unit* victim) const
{
	if (!Unit::canCombatWith(victim))
		return false;

	if (victim->isType(TYPEMASK_PLAYER))
	{
		DataPlayer* dPlayer = victim->asPlayer()->getData();
		if (dPlayer->isGM())
			return false;
	}

	if (this->hasUnitState(UNIT_STATE_HIDING)
		&& this->getData()->getConcealmentState() == CONCEALMENT_STATE_CONCEALED
		&& !this->isWithinDist(victim, DISCOVER_CONCEALED_UNIT_DISTANCE))
	{
		return false;
	}

	float dist = std::max((float)ABANDON_ATTACK_DISTANCE, this->getSightDistance());
	if (!this->isWithinDist(victim, dist))
		return false;

	return true;
}

bool Robot::setInCombatWith(Unit* victim)
{
	if (victim == m_combating)
		return false;

	if (m_combating)
		m_combating->removeEnemy(this);
	victim->addEnemy(this);

	m_combating = victim;

	this->addUnitState(UNIT_STATE_IN_COMBAT);
	this->addThreat(victim, UNITTHREAT_RECEIVED_DAMAGE, 0.f);

	return true;
}

bool Robot::removeCombatTarget()
{
	if (!m_combating)
		return false;

	m_combating->removeEnemy(this);
	m_combating = nullptr;

	return true;
}

void Robot::combatStop()
{
	this->removeCombatTarget();

	if (this->getAI())
	{
		if (this->getAI()->getType() == ROBOTAI_TYPE_SPARRING)
			this->setCombatState(COMBAT_STATE_ESCAPE);
		else
			this->setCombatState(COMBAT_STATE_CHASE);
		this->getAI()->clearAIAction(AI_ACTION_TYPE_COMBAT);
	}

	this->chargeStop();

	Unit::combatStop();
}

float Robot::calcOptimalDistanceToDodgeTarget(Unit* target) const
{
	// 减去TILE_WIDTH_HALF防止走位死循环
	float dist = this->calcEffectiveRange(target) - TILE_WIDTH_HALF;
	return dist;
}

float Robot::calcOptimalDistanceToDodgeTarget(ItemBox* target) const
{
	float dist = this->calcEffectiveRange(target) - TILE_WIDTH_HALF;
	dist = std::min(ATTACK_FAST_WITHIN_DIST, dist);

	return dist;
}

bool Robot::checkCombatChaseConditions() const
{
	NS_ASSERT(m_combating);
	MapData* mapData = m_map->getMapData();

	if (!this->canActiveAttack(m_combating))
		return false;

	// 与危险区已保持安全距离
	if (m_map->getCurrentSafeZoneRadius() > 0)
	{
		TileCoord currdCoord(mapData->getMapSize(), this->getData()->getPosition());
		if (!m_map->isSafeDistanceMaintained(currdCoord))
			return false;

		TileCoord targetCoord(mapData->getMapSize(), m_combating->getData()->getPosition());
		if (!m_map->isWithinSafeZone(targetCoord))
			return false;
	}

	RobotNature const* nature = sObjectMgr->getRobotNature(this->getData()->getNatureType());
	float healthPercent = this->getData()->getHealth() / (float)this->getData()->getMaxHealth() * 100;
	if (healthPercent > nature->getEffectValue(NATURE_EFFECT_ABANDON_ESCAPE_TO_CHASE_HEALTH_PERCENT))
		return true;

	return false;
}

bool Robot::checkCombatEscapeConditions() const
{
	NS_ASSERT(m_combating);
	MapData* mapData = m_map->getMapData();

	if (!this->canActiveAttack(m_combating))
		return true;

	// 在危险中
	if (m_map->getCurrentSafeZoneRadius() > 0)
	{
		if (m_dangerState == DANGER_STATE_ENTERING 
			&& m_dangerStateTimer.getRemainder() <= m_dangerStateTimer.getDuration() / 2.f)
		{
			return true;
		}
	}

	float healthPercent = this->getData()->getHealth() / (float)this->getData()->getMaxHealth() * 100;
	RobotNature const* nature = sObjectMgr->getRobotNature(this->getData()->getNatureType());
	if (healthPercent > nature->getEffectValue(NATURE_EFFECT_ABANDON_ESCAPE_HEALTH_PERCENT))
		return false;

	// 一对一的情况下生命值比例大于阈值并且生命值高于对手
	if (m_unitThreatManager.getThreatList().size() <= 1
		&& healthPercent > nature->getEffectValue(NATURE_EFFECT_ABANDON_ESCAPE_IN_1V1_COMBAT_HEALTH_PERCENT)
		&& this->getData()->getHealth() > m_combating->getData()->getHealth())
	{
		return false;
	}

	return true;
}

bool Robot::updateCombatState()
{
	if (!this->isInCombat())
		return false;

	switch (m_combatState)
	{
	case COMBAT_STATE_CHASE:
		if (this->checkCombatEscapeConditions())
		{
			m_combatState = COMBAT_STATE_ESCAPE;
			return true;
		}
		break;
	case COMBAT_STATE_ESCAPE:
		if (this->checkCombatChaseConditions())
		{
			m_combatState = COMBAT_STATE_CHASE;
			return true;
		}
		break;
	default:
		break;
	}

	return false;
}

bool Robot::canActiveAttack(Unit* victim) const
{
	if (this->hasWorldExplored())
		return true;

	if (this->isInCollection())
		return true;

	// 危险警报未触发前不主动攻击玩家
	if (victim->isType(TYPEMASK_PLAYER) && !m_map->isDangerAlertTriggered())
		return false;

	// 魔豆数量超过敌人
	int32 mbDiff = this->getData()->getMagicBeanCount() - victim->getData()->getMagicBeanCount();
	if (mbDiff >= m_favorableMagicBeanDiff)
		return true;

	// 收集到了足够多的装备
	if (m_equipmentCount >= m_favorableEquipmentCount)
		return true;

	return false;
}

void Robot::receiveDamage(Unit* attacker, int32 damage)
{
	Unit::receiveDamage(attacker, damage);
	this->resetHealthRegenTimer();

	if (attacker)
	{
		// 露出并重新隐蔽
		switch (this->getData()->getConcealmentState())
		{
		case CONCEALMENT_STATE_CONCEALED:
			this->expose();
			this->concealDelayed();
			break;
		case CONCEALMENT_STATE_CONCEALING:
			this->resetConcealmentTimer();
			break;
		default:
			break;
		}

		if (this->canCombatWith(attacker))
		{
			// 增加单位威胁后必须立即开始战斗，延迟战斗将可能造成威胁被添加但是战斗没有开始的情况（目标已不在可视范围内）
			this->addThreat(attacker, UNITTHREAT_RECEIVED_DAMAGE, (float)damage);

			if (!this->isInCombat())
			{
				if(this->getAI())
					this->getAI()->combatStart(attacker);
			}
		}
	}
}

void Robot::modifyMaxStamina(int32 stamina)
{
	int32 current = std::min(this->getData()->getStamina(), stamina);
	this->getData()->setStamina(current);
	this->getData()->setMaxStamina(stamina);

	if (m_staminaUpdater->isInCharge())
		m_staminaUpdater->chargeUpdate();
	else
		m_staminaUpdater->startRegenStamina();

	m_isAttackModeDirty = true;
}

void Robot::modifyStaminaRegenRate(float regenRate)
{
	this->getData()->setStaminaRegenRate(regenRate);

	if (m_staminaUpdater->isInCharge())
		m_staminaUpdater->chargeUpdate();
	else
		m_staminaUpdater->startRegenStamina();

	m_isAttackModeDirty = true;
}

void Robot::modifyChargeConsumesStamina(int32 stamina)
{
	this->getData()->setChargeConsumesStamina(stamina);

	if (m_staminaUpdater->isInCharge())
		m_staminaUpdater->chargeUpdate();
	else
		m_staminaUpdater->startRegenStamina();

	m_isAttackModeDirty = true;
}

bool Robot::canSeeOrDetect(WorldObject* object) const
{
	if (!object->isInWorld() || !object->isVisible())
		return false;

	if (object == this)
		return false;

	// 当前机器人无法看到其他GM玩家
	if (object->isType(TYPEMASK_PLAYER))
	{
		DataPlayer* dPlayer = object->asPlayer()->getData();
		if (dPlayer->isGM())
			return false;
	}

	if (object->isType(TYPEMASK_UNIT))
	{
		if (!this->hasItemEffectType(ITEM_EFFECT_DISCOVER_CONCEALED_UNIT))
		{
			// 当前机器人无法看到已隐蔽并且不在发现范围内的单位
			DataUnit* dUnit = object->asUnit()->getData();
			if (dUnit->getConcealmentState() == CONCEALMENT_STATE_CONCEALED && !this->isWithinDist(object, DISCOVER_CONCEALED_UNIT_DISTANCE))
				return false;
		}
	}

	if (this->isWithinDist(object, this->getSightDistance()))
		return true;

	return false;
}

bool Robot::isWithinEffectiveRange(AttackableObject* target) const
{
	float dist = this->calcEffectiveRange(target);
	bool ret = this->isWithinDist(target->getData()->getPosition(), dist);
	return  ret;
}

bool Robot::lockingTarget(AttackableObject* target) const
{
	if (!target->hasWatcher(const_cast<Robot*>(this)))
		return false;

	if (!this->isWithinEffectiveRange(target))
		return false;

	if (Unit* unit = target->asUnit())
	{
		if (!this->hasItemEffectType(ITEM_EFFECT_DISCOVER_CONCEALED_UNIT)
			&& unit->getData()->getConcealmentState() == CONCEALMENT_STATE_CONCEALED
			&& !this->isWithinDist(unit, DISCOVER_CONCEALED_UNIT_DISTANCE))
		{
			return false;
		}
	}

	MapData* mapData = this->getData()->getMapData();
	TileCoord myCoord(mapData->getMapSize(), this->getData()->getPosition());
	TileCoord targetCoord(mapData->getMapSize(), target->getData()->getPosition());
	if (myCoord == targetCoord)
		return false;

	return true;

}

float Robot::calcEffectiveRange(AttackableObject* target) const
{
	float dist;
	if (target->isType(TYPEMASK_UNIT) && target->asUnit()->hasUnitState(UNIT_STATE_MOVING))
		dist = this->getData()->getAttackRange();
	else
		dist = this->getData()->getAttackRange() - MISS_DISTANCE / 2;

	return dist;
}

void Robot::cleanupAfterUpdate()
{
	this->getData()->clearUnitFlag(UNIT_FLAG_DAMAGED);

	Unit::cleanupAfterUpdate();
}

void Robot::sendMoveOpcodeToNearbyPlayers(Opcode opcode, MovementInfo& movement)
{
	PlayerRangeExistsObjectFilter filter(this);
	std::list<Player*> result;
	ObjectSearcher<Player, PlayerRangeExistsObjectFilter> searcher(filter, result);

	this->visitNearbyObjectsInMaxVisibleRange(searcher);

	for (auto it = result.begin(); it != result.end(); ++it)
	{
		Player* target = *it;
		if (WorldSession* session = target->getSession())
		{
			WorldPacket packet(opcode);
			movement.time = session->getClientNowTimeMillis();
			session->packAndSend(std::move(packet), movement);
		}

	}

	// NS_LOG_DEBUG("behaviors.robot", "Robot::sendMoveOpcodeToNearbyPlayers: %s", getOpcodeNameForLogging(opcode));
}

void Robot::sendMoveSync()
{
	MovementInfo movement = this->getData()->getMovementInfo();
	if ((movement.flags & MOVEMENT_FLAG_WALKING) != 0)
		movement.position = this->getData()->getMoveSegment().position;
	this->sendMoveOpcodeToNearbyPlayers(MSG_MOVE_SYNC, movement);
}

void Robot::sendStaminaOpcodeToNearbyPlayers(Opcode opcode, StaminaInfo& stamina)
{
	PlayerRangeExistsObjectFilter filter(this);
	std::list<Player*> result;
	ObjectSearcher<Player, PlayerRangeExistsObjectFilter> searcher(filter, result);

	this->visitNearbyObjectsInMaxVisibleRange(searcher);

	for (auto it = result.begin(); it != result.end(); ++it)
	{
		Player* target = *it;
		if (WorldSession* session = target->getSession())
		{
			WorldPacket packet(opcode);
			stamina.time = session->getClientNowTimeMillis();
			session->packAndSend(std::move(packet), stamina);
		}

	}

	//NS_LOG_DEBUG("behaviors.robot", "Robot::sendStaminaOpcodeToNearbyPlayers: %s", getOpcodeNameForLogging(opcode));
}

void Robot::sendLocationInfoToNearbyPlayers(LocationInfo& location)
{
	if (!this->getLocator())
		return;

	PlayerRangeExistsLocatorFilter filter(this->getLocator());
	std::list<Player*> result;
	ObjectSearcher<Player, PlayerRangeExistsLocatorFilter> searcher(filter, result);

	this->visitAllObjects(searcher);

	for (auto it = result.begin(); it != result.end(); ++it)
	{
		Player* target = *it;
		if (WorldSession* session = target->getSession())
		{
			WorldPacket packet(SMSG_RELOCATE_LOCATOR);
			location.time = session->getClientNowTimeMillis();
			session->packAndSend(std::move(packet), location);
		}

	}
}

void Robot::updateDangerState()
{
	if (!this->isAlive() || !m_map->isSafeZoneEnabled())
		return;

	MapData* mapData = m_map->getMapData();
	TileCoord currdCoord(mapData->getMapSize(), this->getData()->getPosition());

	if (!m_map->isWithinSafeZone(currdCoord))
		this->enterDangerState();
	else
		this->releaseDangerState();
}

void Robot::increaseStepCount()
{
	++m_stepCount;
	NS_LOG_DEBUG("behaviors.robot", "steps: %d", m_stepCount);
}

ExplorArea Robot::getExplorArea(Point const& pos) const 
{ 
	return ExplorArea(pos, m_explorAreaSize, m_explorAreaOffset); 
}

bool Robot::isExplorAreaContainsPoint(ExplorArea const& area, Point const& point) const 
{ 
	return area.containsPoint(point, m_explorAreaSize, m_explorAreaOffset); 
}

Point Robot::computeExplorAreaPosition(ExplorArea const& area) const 
{ 
	return area.computePosition(m_explorAreaSize, m_explorAreaOffset); 
}

void Robot::increaseDistrictCounter(TileCoord const& coordInDistrict)
{
	uint32 districtId = m_map->getMapData()->getDistrictId(coordInDistrict);
	if (!m_map->isDangerousDistrict(coordInDistrict))
	{
		++m_districtCounters[districtId];
		NS_LOG_DEBUG("behaviors.robot", "increaseDistrictCounter: %d coord: %d,%d districtId: %d", m_districtCounters[districtId], coordInDistrict.x, coordInDistrict.y, districtId);
	}
	else
	{
		NS_LOG_DEBUG("behaviors.robot", "Unable to increase count for dangerous district. coord: %d,%d districtId: %d", coordInDistrict.x, coordInDistrict.y, districtId);
	}
}

uint32 Robot::getDistrictCount(TileCoord const& coordInDistrict) const
{
	uint32 districtId = m_map->getMapData()->getDistrictId(coordInDistrict);
	auto it = m_districtCounters.find(districtId);
	if (it != m_districtCounters.end())
		return (*it).second;

	return 0;
}

bool Robot::isAllDistrictsExplored() const
{
	return m_districtCounters.size() >= this->getMap()->getExplorableDistrictCount();
}

void Robot::removeDistrictCounter(uint32 districtId)
{
	auto it = m_districtCounters.find(districtId);
	if (it != m_districtCounters.end())
		m_districtCounters.erase(it);
}

bool Robot::selectWaypointExplorArea(ExplorArea const& currArea, ExplorArea& waypointArea, TileCoord& waypoint)
{
	if (this->findBestWaypoint(currArea.originInTile, waypoint))
	{
		TileCoord point;
		this->getSafePointFromWaypointExtent(waypoint, point);
		Point pos = point.computePosition(m_map->getMapData()->getMapSize());
		ExplorArea area(pos, m_explorAreaSize, m_explorAreaOffset);
		area.originInTile = point;
		waypointArea = area;
		return true;
	}

	return false;
}

void Robot::getSafePointFromWaypointExtent(TileCoord const& waypoint, TileCoord& result)
{
	auto const& waypointExtent = m_map->getMapData()->getWaypointExtent(waypoint);
	std::vector<TileCoord> safePointList;
	TileCoord safestPoint = waypoint;
	for (auto const& point : waypointExtent)
	{
		int32 r1 = m_map->calcRadiusToSafeZoneCenter(point);
		int32 r2 = m_map->calcRadiusToSafeZoneCenter(safestPoint);
		if (r1 < r2)
			safestPoint = point;

		if (m_map->isSafeDistanceMaintained(point))
			safePointList.push_back(point);
	}

	if (!safePointList.empty())
	{
		int32 ranIndex = random(0, (int32)safePointList.size() - 1);
		result = safePointList[ranIndex];
	}
	else
		result = safestPoint;
}

void Robot::setSourceWaypoint(TileCoord const& waypoint)
{
	if (m_sourceWaypoint != waypoint)
	{
		if (m_sourceWaypoint == TileCoord::INVALID || !m_map->getMapData()->isSameDistrict(waypoint, m_sourceWaypoint))
			this->increaseDistrictCounter(waypoint);

		m_sourceWaypoint = waypoint;
	}
}

void Robot::resetExploredAreas()
{
	NS_LOG_DEBUG("behaviors.robot", "Reset explored areas.");
	m_explorAreaFlags.clear();
	m_explorOrderCounter = 0;
	m_districtCounters.clear();

	m_districtUnexploredExplorAreas.clear();
	m_unexploredExplorAreaLookupTable.clear();
}

void Robot::markAreaExplored(ExplorArea const& area)
{
	int32 areaIndex = area.x * m_explorHeight + area.y;
	++m_explorOrderCounter;
	m_explorAreaFlags[areaIndex] = m_explorOrderCounter;
}

uint32 Robot::getExplorOrder(ExplorArea const& area)
{
	int32 areaIndex = area.x * m_explorHeight + area.y;
	auto it = m_explorAreaFlags.find(areaIndex);
	if (it != m_explorAreaFlags.end())
		return (*it).second;

	return 0;
}

void Robot::increaseWorldExplorCounter()
{
	++m_worldExplorCount;

	NS_LOG_DEBUG("behaviors.robot", "%s worldExplorCount: %d", this->getData()->getGuid().toString().c_str(), m_worldExplorCount);
}

bool Robot::hasWorldExplored() const
{
	if (m_worldExplorCount > 0)
		return true;

	MapData* mapData = m_map->getMapData();
	TileCoord currdCoord(mapData->getMapSize(), this->getData()->getPosition());
	if (m_map->checkPatrolConditions(currdCoord))
		return true;

	return false;
}

void Robot::addUnexploredExplorArea(ExplorArea const& area)
{
	int32 areaIndex = area.x * m_explorHeight + area.y;
	auto it = m_unexploredExplorAreaLookupTable.find(areaIndex);
	if (it == m_unexploredExplorAreaLookupTable.end())
	{
		uint32 districtId = m_map->getMapData()->getDistrictId(area.originInTile);
		auto& areaList = m_districtUnexploredExplorAreas[districtId];
		auto it = areaList.emplace(areaList.end(), area);
		NS_ASSERT(it != areaList.end());
		auto ret = m_unexploredExplorAreaLookupTable.emplace(areaIndex, it);
		NS_ASSERT(ret.second);
	}
}

void Robot::removeUnexploredExplorArea(ExplorArea const& area)
{
	int32 areaIndex = area.x * m_explorHeight + area.y;
	auto it = m_unexploredExplorAreaLookupTable.find(areaIndex);
	if (it != m_unexploredExplorAreaLookupTable.end())
	{
		auto const& areaIt = (*it).second;
		uint32 districtId = m_map->getMapData()->getDistrictId((*areaIt).originInTile);
		auto& areaList = m_districtUnexploredExplorAreas[districtId];
		NS_ASSERT(!areaList.empty());
		areaList.erase(areaIt);
		m_unexploredExplorAreaLookupTable.erase(it);
	}
}

void Robot::removeAllUnexploredExplorAreas()
{
	m_districtUnexploredExplorAreas.clear();
	m_unexploredExplorAreaLookupTable.clear();
}

void Robot::updateUnexploredExplorAreas(ExplorArea const& currArea)
{
	uint32 districtId = m_map->getMapData()->getDistrictId(currArea.originInTile);
	auto areaListIt = m_districtUnexploredExplorAreas.find(districtId);
	if (areaListIt != m_districtUnexploredExplorAreas.end())
	{
		bool isRemoveAll = m_map->isDangerousDistrict(currArea.originInTile);
		auto& areaList = (*areaListIt).second;
		for (auto it = areaList.begin(); it != areaList.end();)
		{
			auto const& area = *it;
			if (isRemoveAll || !m_map->isSafeDistanceMaintained(area.originInTile) || this->isExplorAreaExcluded(area))
			{
				int32 areaIndex = area.x * m_explorHeight + area.y;
				auto nRemoved = m_unexploredExplorAreaLookupTable.erase(areaIndex);
				NS_ASSERT(nRemoved != 0);
				it = areaList.erase(it);
			}
			else
				++it;
		}
	}
}

bool Robot::selectNextUnexploredExplorArea(ExplorArea const& currArea, ExplorArea& result)
{
	this->updateUnexploredExplorAreas(currArea);

	std::vector<ExplorAreaSortItem> orderedExplorAreas;
	uint32 districtId = m_map->getMapData()->getDistrictId(currArea.originInTile);
	auto const& areaList = m_districtUnexploredExplorAreas[districtId];
	for (auto it = areaList.begin(); it != areaList.end(); ++it)
	{
		auto const& area = *it;
		ExplorAreaSortItem item;
		item.area = area;
		item.distance = currArea.originInTile.getDistance(area.originInTile);
		orderedExplorAreas.emplace_back(item);
	}
	if (orderedExplorAreas.size() > 1)
		std::sort(orderedExplorAreas.begin(), orderedExplorAreas.end(), ExplorAreaSortPred());

	auto it = orderedExplorAreas.begin();
	if (it != orderedExplorAreas.end())
	{
		result = (*it).area;
		return true;
	}

	return false;
}

bool Robot::hasUnexploredExplorArea(ExplorArea const& currArea)
{
	uint32 districtId = m_map->getMapData()->getDistrictId(currArea.originInTile);
	return !m_districtUnexploredExplorAreas[districtId].empty();
}

void Robot::addExcludedExplorArea(ExplorArea const& area)
{
	int32 areaIndex = area.x * m_explorHeight + area.y;
	auto it = m_excludedExplorAreaLookupTable.find(areaIndex);
	NSTime time = (NSTime)(getUptimeMillis() / (int64)1000);
	if (it == m_excludedExplorAreaLookupTable.end())
	{
		auto it = m_excludedExplorAreaList.emplace(m_excludedExplorAreaList.end(), std::make_pair(area, time));
		NS_ASSERT(it != m_excludedExplorAreaList.end());
		auto ret = m_excludedExplorAreaLookupTable.emplace(areaIndex, it);
		NS_ASSERT(ret.second);
	}
	else
	{
		auto& p = *(*it).second;
		p.second = time;
	}
}

bool Robot::isExplorAreaExcluded(ExplorArea const& area) const
{
	int32 areaIndex = area.x * m_explorHeight + area.y;
	auto it = m_excludedExplorAreaLookupTable.find(areaIndex);
	return it != m_excludedExplorAreaLookupTable.end();
}

void Robot::updateExcludedExplorAreas()
{
	for (auto it = m_excludedExplorAreaList.begin(); it != m_excludedExplorAreaList.end(); )
	{
		ExplorArea const& area = (*it).first;
		NSTime time = (*it).second;
		int32 areaIndex = area.x * m_explorHeight + area.y;
		int64 diff = getUptimeMillis() - time * (int64)1000;
		if (diff >= EXCLUDED_EXPLOR_AREA_EXPIRATION_TIME)
		{
			auto nRemoved = m_excludedExplorAreaLookupTable.erase(areaIndex);
			NS_ASSERT(nRemoved != 0);
			it = m_excludedExplorAreaList.erase(it);
		}
		else
			++it;
	}
}

void Robot::initializeAI(RobotAIType type)
{
	if (m_ai)
	{
		if (static_cast<RobotAI*>(m_ai)->getType() == type)
		{
			m_ai->resetAI();
			return;
		}
		else
		{
			delete m_ai;
			m_ai = nullptr;
		}
	}
	switch (type)
	{
	case ROBOTAI_TYPE_NONE:
		break;
	case ROBOTAI_TYPE_TRAINING:
		m_ai = new TrainingRobotAI(this);
		break;
	case ROBOTAI_TYPE_SPARRING:
		m_ai = new SparringRobotAI(this);
		break;
	}
}

void Robot::attackDelayed()
{
	if (m_isAttackDelayed)
		return;

	NS_ASSERT(!m_staminaUpdater->isInCharge());

	this->getData()->addMovementFlag(MOVEMENT_FLAG_HANDUP);
	this->sendMoveSync();

	this->resetAttackLostTime();

	m_attackDelayTimer.setDuration(ATTACK_INTERVAL);
	m_isAttackDelayed = true;

	//NS_LOG_DEBUG("behaviors.robot", "Robot::attackDelayed()");
}

void Robot::attackInternal()
{
	if (!m_attacking)
		return;

	float direction;
	if (m_isContinuousAttack)
	{
		NS_ASSERT(!m_staminaUpdater->isInCharge() && m_attackSpeed == ATTACK_SPEED_NONE);
		if (m_attackDirection == FLT_MAX)
			m_attackDirection = this->calcAimingDirection(m_attacking);
		direction = m_attackDirection;
	}
	else
	{
		if(m_staminaUpdater->isInCharge())
			direction = this->calcAimingDirection(m_attacking);
		else
			direction = MathTools::computeAngleInRadians(this->getData()->getPosition(), m_attacking->getData()->getPosition());
	}

	this->getData()->setOrientation(direction);

	this->addUnitState(UNIT_STATE_ATTACKING);

	// 露出并重新隐蔽
	switch (this->getData()->getConcealmentState())
	{
	case CONCEALMENT_STATE_CONCEALED:
		this->expose();
		this->concealDelayed();
		break;
	case CONCEALMENT_STATE_CONCEALING:
		this->resetConcealmentTimer();
		break;
	default:
		break;
	}

	SimpleProjectile simpleProj;
	simpleProj.launcher = this->getData()->getGuid();
	simpleProj.launcherOrigin = this->getData()->getPosition();
	simpleProj.attackRange = this->getData()->getAttackRange();
	simpleProj.launchCenter = this->getData()->getLaunchCenter();
	simpleProj.launchRadiusInMap = this->getData()->getLaunchRadiusInMap();
	simpleProj.orientation = direction;
	simpleProj.chargedStamina = this->getData()->getChargedStamina();

	// 抛射体类型
	simpleProj.projectileType = PROJECTILE_TYPE_NORMAL;
	if (m_staminaUpdater->isInCharge())
		simpleProj.projectileType = PROJECTILE_TYPE_CHARGED;
	else
	{
		if (this->hasItemEffectType(ITEM_EFFECT_ENHANCE_PROJECTILE))
		{
			float chance = (float)getTotalItemEffectModifier(ITEM_EFFECT_ENHANCE_PROJECTILE);
			if (rollChance(chance))
				simpleProj.projectileType = PROJECTILE_TYPE_INTENSIFIED;
		}
	}

	// 伤害加成
	simpleProj.damageBonusRatio = 0.f;
	if((simpleProj.projectileType == PROJECTILE_TYPE_CHARGED || simpleProj.projectileType == PROJECTILE_TYPE_INTENSIFIED) && this->hasItemEffectType(ITEM_EFFECT_DAMAGE_BONUS_PERCENT))
		simpleProj.damageBonusRatio = (float)getTotalItemEffectModifier(ITEM_EFFECT_DAMAGE_BONUS_PERCENT) / 100.f;

	// 计算抛射体大小比例
	simpleProj.scale = this->calcProjectileScale(simpleProj.projectileType);
	NS_ASSERT(simpleProj.scale >= 1.0f);

	Projectile* proj = m_map->takeReusableObject<Projectile>();
	if (proj)
		proj->reloadData(simpleProj, m_map);
	else
	{
		proj = new Projectile();
		proj->loadData(simpleProj, m_map);
	}
	proj->getLauncher().link(&m_launchRefManager, this);
	this->getMap()->addToMap(proj);
	proj->updateObjectVisibility(true);

	if (!m_staminaUpdater->isInCharge())
	{
		if (m_isContinuousAttack)
		{
			//NS_LOG_DEBUG("behaviors.robot", "Continuous Attack direction: %f", direction);
			--m_numAttacks;
			if (m_numAttacks <= 0
				|| this->getData()->getStamina() < this->getData()->getAttackTakesStamina())
			{
				m_numAttacks = 0;
				m_isContinuousAttack = false;
				m_attackDirection = FLT_MAX;
				m_isAttackModeDirty = true;
			}
			this->resetAttackTimer();
		}
		else
		{
			NS_ASSERT(m_attackSpeed != ATTACK_SPEED_NONE);
			NSTime delay = random(m_attackMinInterval, m_attackMaxInterval);
			this->setAttackTimer(0, delay);
			//NS_LOG_DEBUG("behaviors.robot", "Normal Attack direction: %f nextDelay: %d", direction, delay);
		}
	}
	else
	{
		//NS_LOG_DEBUG("behaviors.robot", "Charged Attack direction: %f", direction);
		m_isAttackModeDirty = true;
	}

	m_staminaUpdater->deductStaminaForAttack();
	this->startHandDownTimer();
}

void Robot::updateAttackTimer(NSTime diff)
{
	if (m_attackSpeed == ATTACK_SPEED_NONE && !m_isContinuousAttack)
		return;

	if (m_attackTimer > 0)
	{
		if (diff < m_attackTimer - m_attackLostTime)
			m_attackTimer -= diff;
		else
		{
			m_attackLostTime = diff - (m_attackTimer - m_attackLostTime);
			m_attackTimer = 0;
		}
	}
}

void Robot::updateAttackMode()
{
	if (!m_isAttackModeDirty)
		return;

	m_isAttackModeDirty = false;

	if (!m_attacking)
		return;

	if (m_isAttackDelayed)
		return;

	if (m_staminaUpdater->isInCharge())
		return;

	if (!m_attacking->isType(TYPEMASK_UNIT)
		|| this->isWithinDist(m_attacking, ATTACK_FAST_WITHIN_DIST)
		|| !m_attacking->asUnit()->hasUnitState(UNIT_STATE_MOVING)
		|| m_isSlowMoveEnabled)
	{
		this->startNormalAttack(ATTACK_SPEED_FAST);
	}
	else
	{
		if (this->isWithinDist(m_attacking, ATTACK_CHARGED_WITHIN_DIST) && this->canCharge())
			this->charge();
		else
		{
			if (!this->startContinuousAttack())
				this->startNormalAttack(ATTACK_SPEED_SLOW);
		}
	}
}

void Robot::stopStaminaUpdate()
{
	m_staminaUpdater->stop();
}

void Robot::concealed()
{
	NS_ASSERT(this->getData()->getConcealmentState() != CONCEALMENT_STATE_CONCEALED);

	// 标记当前的隐藏点为已检查
	TileCoord currCoord(this->getMap()->getMapData()->getMapSize(), this->getData()->getPosition());
	this->markHidingSpotChecked(currCoord);

	Unit::concealed();
}

void Robot::relocateHidingSpot(TileCoord const& currCoord)
{
	MapData* mapData = m_map->getMapData();
	TileCoord hidingSpot;
	if (mapData->findNearestHidingSpot(currCoord, hidingSpot))
	{
		Point hidingSpotPos = hidingSpot.computePosition(mapData->getMapSize());
		if (this->isWithinDist(hidingSpotPos, this->getSightDistance()))
		{
			if(this->getAI())
				this->getAI()->discoverHidingSpot(hidingSpot);
		}
	}
}

void Robot::updateAttackDelayTimer(NSTime diff)
{
	if (!m_isAttackDelayed)
		return;

	m_attackDelayTimer.update(diff);
	if (m_attackDelayTimer.passed())
	{
		this->attackInternal();
		m_attackDelayTimer.reset();
		m_isAttackDelayed = false;
	}
}

void Robot::stopAttackDelayTimer()
{
	m_attackDelayTimer.reset();
	m_isAttackDelayed = false;
}

void Robot::startHandDownTimer()
{
	NS_ASSERT_DEBUG(this->getData()->hasMovementFlag(MOVEMENT_FLAG_HANDUP));

	m_handDownTimer.setDuration(HAND_DOWN_DELAY);
	m_isHandDownTimerEnabled = true;
}

void Robot::stopHandDownTimer()
{
	m_handDownTimer.reset();
	m_isHandDownTimerEnabled = false;
}

void Robot::updateHandDownTimer(NSTime diff)
{
	if (!m_isHandDownTimerEnabled)
		return;

	m_handDownTimer.update(diff);
	if (m_handDownTimer.passed())
	{
		NS_ASSERT_DEBUG(this->getData()->hasMovementFlag(MOVEMENT_FLAG_HANDUP));
		NS_ASSERT_DEBUG(!m_staminaUpdater->isInCharge());
		NS_ASSERT_DEBUG(!m_isAttackDelayed);

		this->getData()->clearMovementFlag(MOVEMENT_FLAG_HANDUP);
		this->sendMoveSync();

		m_handDownTimer.reset();
		m_isHandDownTimerEnabled = false;
	}
}

void Robot::setupExplorAreas(MapData* mapData, Point const& centerInArea)
{
	float tilesWidth = mapData->getMapSize().width * mapData->getTileSize().width;
	float tilesHeight = mapData->getMapSize().height * mapData->getTileSize().height;

	m_explorAreaSize = SIGHT_DISTANCE * 2;
	float halfAreaSize = (float)m_explorAreaSize / 2;

	float offsetX;
	float offsetY;
	float remainderX = std::fmod(centerInArea.x, (float)m_explorAreaSize);
	if (remainderX > halfAreaSize)
		offsetX = m_explorAreaSize - remainderX + halfAreaSize;
	else
		offsetX = halfAreaSize - remainderX;
	float remainderY = std::fmod(centerInArea.y, (float)m_explorAreaSize);
	if (remainderY > halfAreaSize)
		offsetY = m_explorAreaSize - remainderY + halfAreaSize;
	else
		offsetY = halfAreaSize - remainderY;

	m_explorWidth = (int32)std::ceil((tilesWidth + offsetX) / m_explorAreaSize);
	m_explorHeight = (int32)std::ceil((tilesHeight + offsetY) / m_explorAreaSize);
	int32 areasWidth = m_explorWidth * m_explorAreaSize;
	int32 areasHeight = m_explorHeight * m_explorAreaSize;
	m_explorAreaOffset.x = -offsetX;
	m_explorAreaOffset.y = -offsetY;
}

void Robot::cleanupExplorAreas()
{
	m_isInPatrol = false;
	m_stepCount = 0;
	m_worldExplorCount = 0;
	m_sourceWaypoint = TileCoord::INVALID;
	m_goalExplorArea = ExplorArea::INVALID;
	m_currWaypointExplorArea = ExplorArea::INVALID;

	m_excludedExplorAreaList.clear();
	m_excludedExplorAreaLookupTable.clear();

	this->resetExploredAreas();
}

bool Robot::findBestWaypoint(TileCoord const& findCoord, TileCoord& result)
{
	std::vector<WaypointSortItem> orderedWaypoints;
	WaypointNodeMap const* waypointNodes = m_map->getDistrictWaypointNodes(findCoord);
	if (waypointNodes)
	{
		MapData* mapData = m_map->getMapData();
		for (auto const& it : *waypointNodes)
		{
			WaypointNode* node = it.second;
			WaypointSortItem sortItem;
			sortItem.node = node;
			sortItem.districtCount = 0;
			sortItem.isSameDistrict = false;
			sortItem.isLinkedToSourceWaypoint = false;

			TileCoord linkedWaypoint;
			if (mapData->getLinkedWaypoint(node->getPosition(), linkedWaypoint))
			{
				if (m_sourceWaypoint != TileCoord::INVALID)
				{
					sortItem.isLinkedToSourceWaypoint = m_sourceWaypoint == linkedWaypoint;
					sortItem.isSameDistrict = mapData->isSameDistrict(m_sourceWaypoint, linkedWaypoint);
				}
				sortItem.districtCount = this->getDistrictCount(linkedWaypoint);
			}

			Point startPos = findCoord.computePosition(mapData->getMapSize());
			Point endPos = node->getPosition().computePosition(mapData->getMapSize());
			sortItem.distance = startPos.getDistance(endPos);

			orderedWaypoints.emplace_back(sortItem);
		}
		if (orderedWaypoints.size() > 1)
			std::sort(orderedWaypoints.begin(), orderedWaypoints.end(), WaypointSortPred());

		auto it = orderedWaypoints.begin();
		if (it != orderedWaypoints.end())
		{
			result = (*it).node->getPosition();
			return true;
		}
	}
	return false;
}

bool Robot::checkUseItemChaseConditions(CarriedItem* item) const
{
	if (!this->isInCombat() || this->getCombatState() != COMBAT_STATE_CHASE)
		return false;

	return true;
}

bool Robot::checkUseItemFindEnemiesConditions(CarriedItem* item) const
{
	// 是胜负对决并且找不到敌人时使用
	if (m_map->isShowdown() && m_unitThreatManager.getThreatList().empty())
		return true;

	return false;
}

bool Robot::checkUseItemFirstAidConditions(CarriedItem* item) const
{
	float healthPercent = this->getData()->getHealth() / (float)this->getData()->getMaxHealth() * 100;
	RobotNature const* nature = sObjectMgr->getRobotNature(this->getData()->getNatureType());
	if (healthPercent < nature->getEffectValue(NATURE_EFFECT_USEITEM_FIRST_AID_HEALTH_PERCENT))
		return true;

	return false;
}

bool Robot::checkUseItemEscapeConditions(CarriedItem* item) const
{
	if (!this->isInCombat() || this->getCombatState() != COMBAT_STATE_ESCAPE)
		return false;

	float healthPercent = this->getData()->getHealth() / (float)this->getData()->getMaxHealth() * 100;
	RobotNature const* nature = sObjectMgr->getRobotNature(this->getData()->getNatureType());
	if(healthPercent < nature->getEffectValue(NATURE_EFFECT_USEITEM_ESCAPE_HEALTH_PERCENT))
		return true;

	return false;
}

void Robot::createDroppedItem(ItemTemplate const* tmpl, TileCoord const& spawnPoint, int32 count)
{
	Unit::createDroppedItem(tmpl, spawnPoint, count);
	NS_LOG_DEBUG("behaviors.robot", "DroppedItem ItemId: %d Count: %d SpawnPoint: %d,%d", tmpl->id, count, spawnPoint.x, spawnPoint.y);
}

void Robot::updateFavorableEquipmentCount()
{
	SpawnManager* spawnMgr = m_map->getSpawnManager();
	float count = spawnMgr->getClassifiedItemCount(ITEM_CLASS_EQUIPMENT) / (float)m_map->getMapData()->getPopulationCap();
	int32 maxCount = (int32)std::round(count * 2);
	maxCount = std::min(maxCount, EQUIPMENT_SLOT_END - EQUIPMENT_SLOT_START);
	switch (this->getData()->getNatureType())
	{
	case NATURE_CAREFUL:
		m_favorableEquipmentCount = maxCount;
		break;
	case NATURE_CALM:
		m_favorableEquipmentCount = (int32)std::round(maxCount / 2);
		break;
	case NATURE_RASH:
		m_favorableEquipmentCount = (int32)std::round(maxCount / 3);
		break;
	}
}

void Robot::updateFavorableMagicBeanDiff()
{
	SpawnManager* spawnMgr = m_map->getSpawnManager();
	float count = spawnMgr->getClassifiedItemCount(ITEM_CLASS_MAGIC_BEAN) / (float)m_map->getMapData()->getPopulationCap();
	int32 maxCount = (int32)std::round(count * 2);
	maxCount = std::max(1, std::min(maxCount, FAVORABLE_MAGICBEAN_MAX_DIFF));
	switch (this->getData()->getNatureType())
	{
	case NATURE_CAREFUL:
		m_favorableMagicBeanDiff = maxCount;
		break;
	case NATURE_CALM:
		m_favorableMagicBeanDiff = (int32)std::round(maxCount / 2);
		break;
	case NATURE_RASH:
		m_favorableMagicBeanDiff = (int32)std::round(maxCount / 3);
		break;
	}
}

bool Robot::canUnlockInCombat() const
{
	if (m_unitThreatManager.getThreatList().empty())
		return true;

	// 检查生命值，除非敌人是机器人并且不在解锁中
	bool checkHealth = true;
	if (m_combating && m_combating->isType(TYPEMASK_ROBOT))
	{
		Robot* enemy = m_combating->asRobot();
		if (!enemy->isInUnlock())
			checkHealth = false;
	}
	if (checkHealth)
	{
		RobotNature const* nature = sObjectMgr->getRobotNature(this->getData()->getNatureType());
		float healthPercent = this->getData()->getHealth() / (float)this->getData()->getMaxHealth() * 100;
		if (healthPercent < nature->getEffectValue(NATURE_EFFECT_ABANDON_UNLOCKING_HEALTH_PERCENT))
			return false;
	}

	return true;
}

bool Robot::canCollectInCombat() const
{
	if (m_unitThreatManager.getThreatList().empty())
		return true;

	// 检查生命值，除非敌人是机器人并且不在收集中
	bool checkHealth = true;
	if (m_combating && m_combating->isType(TYPEMASK_ROBOT))
	{
		Robot* enemy = m_combating->asRobot();
		if (!enemy->isInCollection())
			checkHealth = false;
	}
	if (checkHealth)
	{
		RobotNature const* nature = sObjectMgr->getRobotNature(this->getData()->getNatureType());
		float healthPercent = this->getData()->getHealth() / (float)this->getData()->getMaxHealth() * 100;
		if (healthPercent < nature->getEffectValue(NATURE_EFFECT_ABANDON_COLLECTING_HEALTH_PERCENT))
			return false;
	}

	return true;
}
