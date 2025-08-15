#include "SparringRobotAI.h"

#include "game/behaviors/Robot.h"
#include "game/behaviors/ItemBox.h"
#include "game/behaviors/Item.h"
#include "game/behaviors/Projectile.h"
#include "game/movement/generators/MovementGenerator.h"
#include "game/world/World.h"
#include "game/world/ObjectMgr.h"
#include "game/utils/MathTools.h"
#include "logging/Log.h"
#include "utilities/Random.h"

// The minimum duration of the battle opening smiley. Unit: milliseconds.
#define OPENING_SMILEY_DURATION_MIN					1000

// The chance of sending an smiley when the robot's state changes
#define SMILEY_CHANCE_COMBAT_CHASE				1.0f
#define SMILEY_CHANCE_COMBAT_ESCAPE				1.0f
#define SMILEY_CHANCE_OPENING					40.0f


SparringRobotAI::SparringRobotAI(Robot* robot) :
	RobotAI(robot),
	m_isReactionTimeDirty(true),
	m_targetSelector(robot),
	m_attackInAction(AI_ACTION_TYPE_NONE)
{
	m_pendingHidingSpots.fill(TileCoord::INVALID);
	m_pendingNoTargetActionFlags.reset();
}


SparringRobotAI::~SparringRobotAI()
{
}

void SparringRobotAI::updateAI(NSTime diff)
{
	this->updateReactionTimer(diff);
	this->updateOpeningSmileyTimer(diff);

	this->updateCollect();
	this->updateHide();
	this->updateCombat();
	this->updateUnlock();
	this->updateExplore();
}

void SparringRobotAI::resetAI()
{
	m_pendingHidingSpots.fill(TileCoord::INVALID);
	m_pendingNoTargetActionFlags.reset();
	m_targetSelector.removeAllTargets();

	m_isReactionTimeDirty = true;
	m_reactionTimer.setDuration(0);

	m_attackInAction = AI_ACTION_TYPE_NONE;
	m_openingSmileyTimer.setDuration(0);

	RobotAI::resetAI();
}

void SparringRobotAI::updateReactionTimer(NSTime diff)
{
	if (!m_me->getMap()->isInBattle() && !m_me->getMap()->isBattleEnding())
		return;

	if (m_isReactionTimeDirty)
	{
		RobotProficiency const* proficiency = sObjectMgr->getRobotProficiency(m_me->getData()->getProficiencyLevel());
		NS_ASSERT(proficiency);
		NSTime dur = random(proficiency->minTargetReactionTime, proficiency->maxTargetReactionTime);
		m_reactionTimer.setDuration(dur);

		m_isReactionTimeDirty = false;
	}

	m_reactionTimer.update(diff);
	if (m_reactionTimer.passed())
	{
		this->doCollect();
		this->doUse();
		this->doHide();
		this->doSeek();
		this->doCombat();
		this->doUnlock();
		this->doExplore();

		m_reactionTimer.reset();
	}
}

void SparringRobotAI::sayOneOfSmileys(std::vector<uint16> const& smileys)
{
	if (m_me->getData()->getSmiley() != SMILEY_NONE)
		return;

	auto index = random(0, (int32)smileys.size() - 1);
	m_me->saySmiley(smileys[index]);
}

void SparringRobotAI::updateOpeningSmileyTimer(NSTime diff)
{
	if (m_openingSmileyTimer.getDuration() <= 0)
		return;

	m_openingSmileyTimer.update(diff);
	if (m_openingSmileyTimer.passed())
	{
		this->sayOneOfSmileys({ SMILEY_LAUGH, SMILEY_NAUGHTY, SMILEY_DEVIL });
		m_openingSmileyTimer.setDuration(0);
	}
}


void SparringRobotAI::updateCollect()
{
	if (!m_me->isInCollection())
		return;

	Item* wish = m_me->selectWish();
	if (wish)
		this->collectStart(wish);
	else
	{
		m_me->deleteWishList();
		m_me->collectStop();
		m_me->updateObjectVisibility();
	}
}

void SparringRobotAI::updateHide()
{
	if (!m_me->hasUnitState(UNIT_STATE_HIDING))
		return;

	TileCoord const& hidingSpot = m_me->getHidingSpot();
	if (this->getAIAction() > AI_ACTION_TYPE_HIDE || !m_me->canHide(hidingSpot))
		m_me->hideStop();
}

void SparringRobotAI::updateCombat()
{
	if (!m_me->isInCombat())
		return;

	Unit* victim = m_me->selectVictim();
	if (victim)
	{
		this->combatStart(victim);
		this->attackStart(victim, AI_ACTION_TYPE_COMBAT);
	}
	else
	{
		this->clearAttackInAction(AI_ACTION_TYPE_COMBAT);
		m_me->deleteUnitThreatList();
		m_me->combatStop();
		// When both oneself and nearby targets are stationary, updating the state of visibility 
		// allows one to actively search for targets within one's line of sight
		m_me->updateObjectVisibility();
	}
}

void SparringRobotAI::updateUnlock()
{
	if (!m_me->isInUnlock())
		return;

	ItemBox* itemBox = m_me->getUnlockTarget();
	if (itemBox && m_me->canUnlock(itemBox))
	{
		this->unlockStart(itemBox);
		this->attackStart(itemBox, AI_ACTION_TYPE_UNLOCK);
	}
	else
	{
		this->clearAttackInAction(AI_ACTION_TYPE_UNLOCK);
		m_me->unlockStop();
	}
}

void SparringRobotAI::updateExplore()
{
	if (!m_me->isInExploration())
		return;

	if (this->getAIAction() < AI_ACTION_TYPE_EXPLORE)
	{
		if (m_me->canExploreArea())
			this->exploreStart();
	}
	else
	{
		if (!m_me->canExploreArea())
			m_me->exploreStop();
	}
}

void SparringRobotAI::attackStart(AttackableObject* target, AIActionType action)
{
	if (!target || m_attackInAction > action)
		return;

	m_me->setAttackTarget(target);
	if (m_me->lockingTarget(target))
	{
		if (m_me->isAttackReady())
			m_me->attack();
	}
	m_attackInAction = action;
}

void SparringRobotAI::clearAttackInAction(AIActionType action)
{
	if (m_attackInAction == action)
		m_attackInAction = AI_ACTION_TYPE_NONE;
}

void SparringRobotAI::setPendingHidingSpot(TileCoord const& hidingSpot, HidingSpotPurpose purpose)
{
	m_pendingHidingSpots[purpose] = hidingSpot;
}

void SparringRobotAI::setPendingNoTargetAction(NoTargetAction action)
{
	m_pendingNoTargetActionFlags.set(action);
}

void SparringRobotAI::doUse()
{
	for (int32 i = INVENTORY_SLOT_START; i < INVENTORY_SLOT_END; ++i)
	{
		CarriedItem* item = m_me->getItem(i);
		if (!item)
			continue;

		if (m_me->canUseItem(item))
		{
			m_me->useItem(item);
			m_isReactionTimeDirty = true;
		}
	}
}

void SparringRobotAI::doCollect()
{
	WorldObject* target = m_targetSelector.getVaildTarget(TARGET_ACTION_COLLECT);
	if (!target)
		return;

	Item* item = target->asItem();
	if (m_me->getSupply())
		m_me->addWish(item);
	else
		this->collectStart(item);
	m_targetSelector.removeTarget(TARGET_ACTION_COLLECT, item);

	m_isReactionTimeDirty = true;
}

void SparringRobotAI::doHide()
{
	TileCoord const& tileCoord = m_pendingHidingSpots[HIDING_SPOT_FOR_HIDE];
	if (tileCoord.isInvalid())
		return;

	NS_ASSERT(!m_me->hasUnitState(UNIT_STATE_HIDING));
	if (m_me->canHide(tileCoord))
		this->hideStart(tileCoord);

	m_pendingHidingSpots[HIDING_SPOT_FOR_HIDE] = TileCoord::INVALID;
	m_isReactionTimeDirty = true;
}

void SparringRobotAI::doCombat()
{
	WorldObject* target = m_targetSelector.getVaildTarget(TARGET_ACTION_COMBAT);
	if (!target)
		return;

	Unit* unit = target->asUnit();
	if (m_me->getVictim())
	{
		if (!m_me->isHostileTarget(unit))
			m_me->addThreat(unit, UNITTHREAT_RECEIVED_DAMAGE, 0.f);
	}
	else
		this->combatStart(unit);

	m_isReactionTimeDirty = true;
}

void SparringRobotAI::doUnlock()
{
	WorldObject* target = m_targetSelector.getVaildTarget(TARGET_ACTION_UNLOCK);
	if (!target)
		return;

	ItemBox* itemBox = target->asItemBox();
	this->unlockStart(itemBox);
	m_targetSelector.removeTargetsForAction(TARGET_ACTION_UNLOCK);

	m_isReactionTimeDirty = true;
}

void SparringRobotAI::doSeek()
{
	TileCoord const& tileCoord = m_pendingHidingSpots[HIDING_SPOT_FOR_SEEK];
	if (tileCoord.isInvalid())
		return;

	NS_ASSERT(!m_me->hasUnitState(UNIT_STATE_SEEKING));
	if (m_me->canSeek(tileCoord))
		this->seekStart(tileCoord);

	m_pendingHidingSpots[HIDING_SPOT_FOR_SEEK] = TileCoord::INVALID;
	m_isReactionTimeDirty = true;
}

void SparringRobotAI::doExplore()
{
	if (!m_pendingNoTargetActionFlags.test(NOTARGET_ACTION_EXPLORE))
		return;

	if (!m_me->isInExploration() && m_me->canExploreArea())
		this->exploreStart();

	m_pendingNoTargetActionFlags.set(NOTARGET_ACTION_EXPLORE, false);
	m_isReactionTimeDirty = true;
}

void SparringRobotAI::combatDelayed(Unit* victim)
{
	m_targetSelector.addTarget(victim, TARGET_ACTION_COMBAT);
}

void SparringRobotAI::combatStart(Unit* victim)
{
	bool targeted = m_me->setInCombatWith(victim);
	if (((m_me->updateCombatState() || targeted) && this->getAIAction() == AI_ACTION_TYPE_COMBAT) || this->getAIAction() < AI_ACTION_TYPE_COMBAT)
		this->applyCombatMotion(victim);
}

void SparringRobotAI::exploreDelayed()
{
	this->setPendingNoTargetAction(NOTARGET_ACTION_EXPLORE);
}

void SparringRobotAI::applyCombatMotion(Unit* target)
{
	switch (m_me->getCombatState())
	{
	case COMBAT_STATE_CHASE:
		m_me->getMotionMaster()->moveChase(target, true);
		if(rollChance(SMILEY_CHANCE_COMBAT_CHASE))
			this->sayOneOfSmileys({ SMILEY_NAUGHTY });
		break;
	case COMBAT_STATE_ESCAPE:
		m_me->getMotionMaster()->moveEscape(target);
		if (rollChance(SMILEY_CHANCE_COMBAT_ESCAPE))
			this->sayOneOfSmileys({ SMILEY_SAD });
		break;
	default:
		break;
	}

	this->setAIAction(AI_ACTION_TYPE_COMBAT);
	m_me->getData()->setAIActionState(m_me->getCombatState());
}

void SparringRobotAI::unlockDelayed(ItemBox* itemBox)
{
	m_targetSelector.addTarget(itemBox, TARGET_ACTION_UNLOCK);
}

void SparringRobotAI::unlockStart(ItemBox* itemBox)
{
	bool targeted = m_me->setInUnlock(itemBox); 
	bool stateChanged = m_me->updateUnlockState();
	AIActionType targetAction;
	if(m_me->getUnlockState() == UNLOCK_STATE_CHASE)
		targetAction = AI_ACTION_TYPE_UNLOCK;
	else
		targetAction = AI_ACTION_TYPE_UNLOCK_DIRECTLY;

	if ((targeted && this->getAIAction() == targetAction)
		|| (stateChanged && this->getAIAction() == AI_ACTION_TYPE_UNLOCK_DIRECTLY) // Lower the priority of the action
		|| this->getAIAction() < targetAction)
	{
		this->applyUnlockMotion(itemBox, targetAction);
	}
}

void SparringRobotAI::applyUnlockMotion(ItemBox* itemBox, AIActionType action)
{
	switch (m_me->getUnlockState())
	{
	case UNLOCK_STATE_CHASE:
		m_me->getMotionMaster()->moveChase(itemBox, true);
		break;
	case UNLOCK_STATE_GO_BACK:
	{
		TileCoord toCoord(itemBox->getData()->getMapData()->getMapSize(), itemBox->getData()->getPosition());
		m_me->getMotionMaster()->movePoint(toCoord, true);
		break;
	}
	default:
		break;
	}

	this->setAIAction(action);
	m_me->getData()->setAIActionState(m_me->getUnlockState());
}

void SparringRobotAI::collectDelayed(Item* item)
{
	m_targetSelector.addTarget(item, TARGET_ACTION_COLLECT);
}

void SparringRobotAI::collectStart(Item* item)
{
	bool targeted = m_me->setCollectTarget(item);
	bool stateChanged = m_me->updateCollectionState();
	AIActionType targetAction;
	if (m_me->getCollectionState() == COLLECTION_STATE_COLLECT)
		targetAction = AI_ACTION_TYPE_COLLECT_DIRECTLY;
	else
		targetAction = AI_ACTION_TYPE_COLLECT;

	if ((targeted && this->getAIAction() == targetAction)
		|| (stateChanged && this->getAIAction() == AI_ACTION_TYPE_COLLECT_DIRECTLY) // Lower the priority of the action
		|| this->getAIAction() < targetAction)
	{
		this->applyCollectionMotion(item, targetAction);
	}
}

void SparringRobotAI::applyCollectionMotion(Item* item, AIActionType action)
{
	switch (m_me->getCollectionState())
	{
	case COLLECTION_STATE_COLLECT:
	{
		TileCoord toCoord(item->getData()->getMapData()->getMapSize(), item->getData()->getPosition());
		m_me->getMotionMaster()->movePoint(toCoord, true);
		break;
	}
	case COLLECTION_STATE_COMBAT:
		break;
	default:
		break;
	}

	this->setAIAction(action);
	m_me->getData()->setAIActionState(m_me->getCollectionState());
}

void SparringRobotAI::discoverSpotToHide(TileCoord const& tileCoord)
{
	if (m_me->getHidingSpot().isInvalid())
	{
		if (m_me->canHide(tileCoord))
			this->setPendingHidingSpot(tileCoord, HIDING_SPOT_FOR_HIDE);
	}
}

void SparringRobotAI::hideStart(TileCoord const& hidingSpot)
{
	bool chosen = m_me->setHidingSpot(hidingSpot);
	if ((chosen && this->getAIAction() == AI_ACTION_TYPE_HIDE) || this->getAIAction() < AI_ACTION_TYPE_HIDE)
	{
		m_me->getMotionMaster()->movePoint(hidingSpot);
		this->setAIAction(AI_ACTION_TYPE_HIDE);
	}
}

void SparringRobotAI::discoverSpotToSeek(TileCoord const& tileCoord)
{
	if (!m_me->hasUnitState(UNIT_STATE_SEEKING))
	{
		if (m_me->canSeek(tileCoord))
			this->setPendingHidingSpot(tileCoord, HIDING_SPOT_FOR_SEEK);
	}
}

void SparringRobotAI::seekStart(TileCoord const& hidingSpot)
{
	if (this->getAIAction() <= AI_ACTION_TYPE_SEEK)
	{
		m_me->getMotionMaster()->moveSeek(hidingSpot);
		m_me->addUnitState(UNIT_STATE_SEEKING);
		this->setAIAction(AI_ACTION_TYPE_SEEK);
	}
}

void SparringRobotAI::exploreStart()
{
	m_me->addUnitState(UNIT_STATE_IN_EXPLORATION);
	if (this->getAIAction() <= AI_ACTION_TYPE_EXPLORE)
	{
		m_me->getMotionMaster()->moveExplore();
		this->setAIAction(AI_ACTION_TYPE_EXPLORE);
	}
}

void SparringRobotAI::moveInLineOfSight(Unit* who)
{
	if (m_me->getData()->hasReactFlag(REACT_FLAG_PASSIVE_AGGRESSIVE)
		|| m_me->isHostileTarget(who)
		|| !m_me->isPotentialThreat(who))
	{
		return;
	}

	if (m_me->canCombatWith(who))
		this->combatDelayed(who);
}

void SparringRobotAI::moveInLineOfSight(ItemBox* itemBox)
{
	if (m_me->getUnlockTarget())
		return;

	if (m_me->canUnlock(itemBox))
		this->unlockDelayed(itemBox);
}

void SparringRobotAI::moveInLineOfSight(Item* item)
{
	if (m_me->isInterestedTarget(item))
		return;

	if (m_me->canCollect(item))
		this->collectDelayed(item);
}

void SparringRobotAI::moveInLineOfSight(Projectile* proj)
{
	if (m_me->isHostileTarget(proj) || !m_me->isPotentialThreat(proj))
		return;

	m_me->addThreat(proj);
}

void SparringRobotAI::discoverHidingSpot(TileCoord const& tileCoord)
{
	this->discoverSpotToHide(tileCoord);
	this->discoverSpotToSeek(tileCoord);
}

void SparringRobotAI::sayOpeningSmileyIfNeeded()
{
	if (rollChance(SMILEY_CHANCE_OPENING))
	{
		NSTime prepDur = sWorld->getIntConfig(CONFIG_BATTLE_PREPARATION_DURATION);
		NSTime dur = random(OPENING_SMILEY_DURATION_MIN, std::max(OPENING_SMILEY_DURATION_MIN, prepDur));
		m_openingSmileyTimer.setDuration(dur);
	}
}