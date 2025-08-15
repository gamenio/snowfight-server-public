#include "Player.h"

#include "game/server/protocol/pb/RewardMessage.pb.h"
#include "game/server/protocol/pb/BattleResult.pb.h"
#include "game/server/protocol/pb/LaunchResult.pb.h"
#include "game/server/protocol/pb/ItemUseResult.pb.h"
#include "game/server/protocol/pb/ItemPickupResult.pb.h"

#include "utilities/StringUtil.h"
#include "game/utils/MathTools.h"
#include "game/grids/GridNotifiers.h"
#include "game/grids/ObjectSearcher.h"
#include "game/world/ObjectMgr.h"
#include "game/world/ObjectAccessor.h"
#include "game/world/World.h"
#include "game/theater/Theater.h"
#include "UnitHelper.h"
#include "ObjectShapes.h"
#include "UnitLocator.h"

#define ATTACK_INTERVAL								100		// Attack interval time. Unit: milliseconds
#define UNLOCKER_EXPIRATION_TIME					15000	// Unlocker expiration time. Unit: milliseconds

int32 RewardForRanking::getMoney(int32 rankNo, int32 rankTotal) const
{
	int32 topPlayers = (int32)(rankTotal * moneyRewardedPlayersPercent / 100.f);
	topPlayers = std::max(2, topPlayers);
	if (rankNo > topPlayers)
		return 0;

	int32 lastRankNo = std::min(topPlayers, rankTotal);
	uint32 level = (uint32)std::max(1, lastRankNo - (rankNo - 1));
	ValueBuilder<int32> builder(level, topPlayers, 0.f, baseMoney, maxMoney, 0);
	return builder;
}

int32 RewardForRanking::getXP(int32 rankNo, int32 rankTotal) const
{
	int32 topPlayers = (int32)(rankTotal * xpRewardedPlayersPercent / 100.f);
	topPlayers = std::max(2, topPlayers);
	if (rankNo > topPlayers)
		return 0;

	int32 lastRankNo = std::min(topPlayers, rankTotal);
	uint32 level = (uint32)std::max(1, lastRankNo - (rankNo - 1));
	ValueBuilder<int32> builder(level, topPlayers, 0.f, baseXP, maxXP, 0);
	return builder;
}


Player::Player(WorldSession* session):
	m_session(session),
	m_templateId(0),
	m_numAttacks(0),
	m_isContinuousAttack(false),
	m_attackDirection(0),
	m_attackTimer(true),
	m_hitCount(0),
	m_projectionCount(0),
	m_normalAttackCount(0),
	m_chargedAttackCount(0),
	m_continuousAttackCount(0),
	m_earnedXP(0),
	m_itemBoxCount(0),
	m_magicBeanCount(0)
{
	m_type |= TYPEMASK_PLAYER;
	m_typeId = TypeID::TYPEID_PLAYER;

	m_ai = new PlayerAI(this);
}


Player::~Player()
{
	m_session = nullptr;

	if (m_ai)
	{
		delete m_ai;
		m_ai = nullptr;
	}

	if(BattleMap* map = this->getMap())
		map->getPlayerList().erase(this->getData()->getGuid());
}

void Player::update(NSTime diff)
{
	Unit::update(diff);

	if (!this->isInWorld())
		return;

	if (this->isAlive())
	{
		m_ai->updateAI(diff);
		this->updateAttackTimer(diff);
		this->updateUnlockerTimer(diff);
	}
	this->updateWithdrawalTimer(diff);
}


bool Player::loadData(uint32 templateId)
{
	PlayerTemplate const* tmpl = sObjectMgr->getPlayerTemplate(templateId);
	if (!tmpl)
		return false;

	m_templateId = templateId;

	DataPlayer* data = new DataPlayer();
	data->setGuid(ObjectGuid(GUIDTYPE_PLAYER, sObjectMgr->generatePlayerSpawnId()));

	data->setObjectSize(UNIT_OBJECT_SIZE);
	data->setAnchorPoint(UNIT_ANCHOR_POINT);
	data->setObjectRadiusInMap(UNIT_OBJECT_RADIUS_IN_MAP);
	data->setLaunchCenter(UNIT_LAUNCH_CENTER);
	data->setLaunchRadiusInMap(UNIT_LAUNCH_RADIUS_IN_MAP);

	data->setDisplayId(tmpl->displayId);
	data->setPickupDuration(ITEM_PICKUP_DURATION);

	this->setData(data);
	m_session->setPlayer(this);

	if (this->getLocator())
	{
		DataUnitLocator* dataLocator = new DataUnitLocator();
		dataLocator->setDisplayId(data->getDisplayId());
		this->getLocator()->setData(dataLocator);
	}

	return true;

}

void Player::setMap(BattleMap* map)
{
	Unit::setMap(map);
	map->getPlayerList()[this->getData()->getGuid()] = this;
}

void Player::modifyMaxStamina(int32 stamina)
{
	int32 current = std::min(this->getData()->getStamina(), stamina);
	this->getData()->setStamina(current);
	this->getData()->setMaxStamina(stamina);
	this->getData()->increaseStaminaCounter();

	if (this->getSession())
	{
		WorldPacket packet(SMSG_MODIFY_STAMINA);
		this->getSession()->packAndSend(std::move(packet), this->getData()->getStaminaInfo());
	}
}

void Player::modifyStaminaRegenRate(float regenRate)
{
	this->getData()->setStaminaRegenRate(regenRate);
	this->getData()->increaseStaminaCounter();

	if (this->getSession())
	{
		WorldPacket packet(SMSG_MODIFY_STAMINA);
		this->getSession()->packAndSend(std::move(packet), this->getData()->getStaminaInfo());
	}
}

void Player::modifyChargeConsumesStamina(int32 stamina)
{
	this->getData()->setChargeConsumesStamina(stamina);
	this->getData()->increaseStaminaCounter();

	if (this->getSession())
	{
		WorldPacket packet(SMSG_MODIFY_STAMINA);
		this->getSession()->packAndSend(std::move(packet), this->getData()->getStaminaInfo());
	}
}

bool Player::canSeeOrDetect(WorldObject* object) const
{
	if (!object->isInWorld() || !object->isVisible())
		return false;

	if (object == this)
		return true;

	// If the current player has not enabled GM mode, it is not possible to see other GM players
	if (object->isType(TYPEMASK_PLAYER))
	{
		DataPlayer* dPlayer = object->asPlayer()->getData();
		if (dPlayer->isGM() && !this->getData()->isGM())
			return false;
	}

	if (object->isType(TYPEMASK_UNIT))
	{
		if (!this->hasItemEffectType(ITEM_EFFECT_DISCOVER_CONCEALED_UNIT))
		{
			// Current player can't see units that are concealed and not within discover distance
			DataUnit* dUnit = object->asUnit()->getData();
			if (dUnit->getConcealmentState() == CONCEALMENT_STATE_CONCEALED && !this->isWithinDist(object, DISCOVER_CONCEALED_UNIT_DISTANCE))
				return false;
		}
	}

	// If the current player has not enabled GM mode, it will not be able to see other GM players' projectiles
	if (object->isType(TYPEMASK_PROJECTILE))
	{
		Projectile* proj = object->asProjectile();
		if (proj->getLauncher().isValid())
		{
			Unit* launcher = proj->getLauncher().getSource();
			if (launcher->isType(TYPEMASK_PLAYER))
			{
				DataPlayer* dPlayer = launcher->asPlayer()->getData();
				if (dPlayer->isGM() && !this->getData()->isGM())
					return false;
			}
		}
	}

	if (this->isWithinVisibleRange(object))
		return true;

	return false;
}

bool Player::isWithinVisibleRange(WorldObject* object) const
{
	Size visRange = this->getData()->getVisibleRange();
	Size mapSize = this->getData()->getMapData()->getMapSize();
	Point center(this->getData()->getPosition());

	float halfWidth = visRange.width * 0.5f;
	float halfHeight = visRange.height * 0.5f;


	Rect r(center.x - halfWidth,
		center.y - halfHeight,
		visRange.width,
		visRange.height);

	bool b = r.containsPoint(object->getData()->getPosition());

	//NS_LOG_DEBUG("behaviors.player", "player: %s [%f, %f] target: %s [%f, %f] visiblerange: [%f, %f, %f, %f] contains: %d", 
	//	this->getData()->getGuid().toString().c_str(),
	//	this->getData()->getPosition().x, this->getData()->getPosition().y,
	//	object->getData()->getGuid().toString().c_str(),
	//	object->getData()->getPosition().x, object->getData()->getPosition().y,
	//	r.origin.x, r.origin.y, r.size.width, r.size.height, 
	//	b);

	return b;
}

bool Player::canTrack(WorldObject* object) const
{
	if (!object->isInWorld() || !object->isInWorld() || !object->isVisible()
		|| !object->getLocator() || !object->getLocator()->isInWorld())
	{
		return false;
	}

	if (object == this)
		return false;

	if (object->isType(TYPEMASK_UNIT))
	{
		Unit* unit = object->asUnit();

		// If the current player has not enabled GM mode, it is not possible to track other GM players
		if (Player* player = unit->asPlayer())
		{
			DataPlayer* dPlayer = player->getData();
			if (dPlayer->isGM() && !this->getData()->isGM())
				return false;
		}
	}

	if (this->isWithinVisibleRange(object))
		return false;

	return true;
}

bool Player::hasTrackingAtClient(WorldObject* object) const
{
	return (object != this && m_clientTrackingGuids.find(object->getData()->getGuid()) != m_clientTrackingGuids.end());
}

void Player::removeTrackingFromClient(WorldObject* object)
{
	m_clientTrackingGuids.erase(object->getData()->getGuid());
}

void Player::updateTraceabilityOf(WorldObject* target)
{
	if (this->hasTrackingAtClient(target))
	{
		if (!this->canTrack(target))
		{
			if (target->getLocator())
				target->getLocator()->sendOutOfRangeUpdateToPlayer(this);
			m_clientTrackingGuids.erase(target->getData()->getGuid());
		}
	}
	else
	{
		if (this->canTrack(target))
		{
			if (target->getLocator())
				target->getLocator()->sendCreateUpdateToPlayer(this);
			m_clientTrackingGuids.insert(target->getData()->getGuid());
		}
	}
}

void Player::updateTraceabilityOf(WorldObject* target, UpdateObject& update)
{
	if (this->hasTrackingAtClient(target))
	{
		if (!this->canTrack(target))
		{
			if (target->getLocator())
				target->getLocator()->buildOutOfRangeUpdateBlock(&update);
			m_clientTrackingGuids.erase(target->getData()->getGuid());
		}
	}
	else
	{
		if (this->canTrack(target))
		{
			if (target->getLocator())
				target->getLocator()->buildCreateUpdateBlockForPlayer(this, &update);
			m_clientTrackingGuids.insert(target->getData()->getGuid());
		}
	}
}

void Player::updateTraceabilityForPlayer()
{
	TrackingNotifier notifier(*this);
	this->visitAllObjects(notifier);

	notifier.sendToSelf();
}

void Player::updateTraceabilityInClient()
{
	UpdateObject update;
	for (auto it = m_clientTrackingGuids.begin(); it != m_clientTrackingGuids.end(); )
	{
		WorldObject* target = ObjectAccessor::getWorldObject(this, *it);
		if (target && !this->canTrack(target))
		{
			if (target->getLocator())
				target->getLocator()->buildOutOfRangeUpdateBlock(&update);
			it = m_clientTrackingGuids.erase(it);
		}
		else
			++it;
	}

	if (!update.hasUpdate())
		return;

	if (this->getSession())
	{
		WorldPacket packet(SMSG_UPDATE_OBJECT);
		this->getSession()->packAndSend(std::move(packet), update);
	}
}

void Player::attack(AttackInfo const& attackInfo)
{
	//NS_LOG_DEBUG("behaviors.player", "ATTACK facing angle:%f", attackInfo.direction());
	this->addUnitState(UNIT_STATE_ATTACKING);

	// Expose and reconceal
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

	uint32 attackInfoCounter = this->getData()->getAttackInfoCounter();
	if (!this->hasUnitState(UNIT_STATE_CHARGING)
		&& (attackInfo.flags() & ATTACK_FLAG_ALL_OUT) != 0)
	{
		int32 nAttacks = this->getData()->getStamina() / this->getData()->getAttackTakesStamina();
		if (nAttacks > 1)
		{
			int32 regenPoints = (int32)(this->getData()->getStaminaRegenRate() * this->getData()->getMaxStamina() * (nAttacks * (ATTACK_INTERVAL / 1000.f)));
			nAttacks += regenPoints / this->getData()->getAttackTakesStamina();

			m_numAttacks = nAttacks - 1; // Subtract this attack
			m_attackTimer.setInterval(ATTACK_INTERVAL);
			m_isContinuousAttack = true;
			m_attackDirection = attackInfo.direction();
			attackInfoCounter = 0;

			m_continuousAttackCount++;
		}
	}
	else
	{
		if (this->hasUnitState(UNIT_STATE_CHARGING))
			m_chargedAttackCount++;
		else
			m_normalAttackCount++;
	}
	this->attack(attackInfo.direction(), attackInfoCounter);

	if(this->hasUnitState(UNIT_STATE_CHARGING))
		this->chargeStop();		
}

bool Player::attackStop()
{
	this->stopContinuousAttack();
	this->getData()->clearMovementFlag(MOVEMENT_FLAG_HANDUP);
	this->clearUnitState(UNIT_STATE_ATTACKING);
	return true;
}

void Player::stopContinuousAttack()
{
	if (!m_isContinuousAttack)
		return;

	this->resetAttackTimer();
	m_isContinuousAttack = false;
	m_numAttacks = 0;
	m_attackDirection = 0;
}

void Player::charge()
{
	this->stopContinuousAttack();

	this->addUnitState(UNIT_STATE_CHARGING);
	this->getUnitHostileRefManager()->updateThreat(UNITTHREAT_ENEMY_CHARGED_POWER);
	this->updateObjectVisibility();
}

void Player::chargeStop()
{
	this->getData()->setChargeStartStamina(0);
	this->getData()->setChargedStamina(0);
	this->getUnitHostileRefManager()->updateThreat(UNITTHREAT_ENEMY_CHARGED_POWER);

	this->clearUnitState(UNIT_STATE_CHARGING);
}

void Player::stopStaminaUpdate()
{
	this->chargeStop();
}

void Player::receiveDamage(Unit* attacker, int32 damage)
{
	Unit::receiveDamage(attacker, damage);
	if (!m_map->isTrainingGround())
		this->resetHealthRegenTimer();

	if (attacker)
	{
		// Expose and reconceal
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
	}
}

void Player::dealDamage(Unit* victim, int32 damage)
{
	Unit::dealDamage(victim, damage);

	++m_hitCount;
}

void Player::dealDamage(ItemBox* itemBox, int32 damage)
{
	Unit::dealDamage(itemBox, damage);

	if (itemBox->isLocked())
	{
		this->setInUnlock(itemBox);
		this->resetUnlockerTimer();
	}
}

bool Player::canCombatWith(Unit* victim) const
{
	if (!Unit::canCombatWith(victim))
		return false;

	// GM can only attack GM, not other units
	if (this->getData()->isGM()
		&& !(victim->asPlayer() && victim->asPlayer()->getData()->isGM()))
		return false;

	return true;
}

void Player::updateVisibilityForPlayer()
{
	// Update the visibility of all objects within the current player's line of sight
	VisibleNotifier notifier(*this);
	this->visitNearbyObjects(this->getData()->getVisibleRange(), notifier);

	notifier.sendToSelf();
}

void Player::updateVisibilityInClient()
{
	UpdateObject update;
	for (auto it = m_clientGuids.begin(); it != m_clientGuids.end(); )
	{
		WorldObject* target = ObjectAccessor::getWorldObject(this, *it);
		if (target && !this->canSeeOrDetect(target))
		{
			target->buildOutOfRangeUpdateBlock(&update);
			it = m_clientGuids.erase(it);
		}
		else
			++it;
	}

	if (!update.hasUpdate())
		return;

	if (this->getSession())
	{
		WorldPacket packet(SMSG_UPDATE_OBJECT);
		this->getSession()->packAndSend(std::move(packet), update);
	}
}

void Player::updateVisibilityOf(WorldObject* target)
{
	if (this->hasAtClient(target))
	{
		if (!this->canSeeOrDetect(target))
		{
			target->sendOutOfRangeUpdateToPlayer(this);
			m_clientGuids.erase(target->getData()->getGuid());
		}
	}
	else
	{
		if (this->canSeeOrDetect(target))
		{
			target->sendCreateUpdateToPlayer(this);
			m_clientGuids.insert(target->getData()->getGuid());

			target->sendInitialVisiblePacketsToPlayer(this);
		}
	}
}

void Player::updateVisibilityOf(WorldObject* target, UpdateObject& update, std::unordered_set<WorldObject*>& visibleNow)
{
	if (this->hasAtClient(target))
	{
		if (!this->canSeeOrDetect(target))
		{
			target->buildOutOfRangeUpdateBlock(&update);
			m_clientGuids.erase(target->getData()->getGuid());
		}
	}
	else
	{
		if (this->canSeeOrDetect(target))
		{
			target->buildCreateUpdateBlockForPlayer(this, &update);
			m_clientGuids.insert(target->getData()->getGuid());
			visibleNow.insert(target);
		}
	}
}

void Player::updateObjectVisibility(bool forced)
{
	Unit::updateObjectVisibility(forced);
}

void Player::updateDangerState()
{
	if (!this->isAlive() || this->getData()->isGM() || !m_map->isSafeZoneEnabled())
		return;

	MapData* mapData = m_map->getMapData();
	TileCoord currdCoord(mapData->getMapSize(), this->getData()->getPosition());

	if (!m_map->isWithinSafeZone(currdCoord))
		this->enterDangerState();
	else
		this->releaseDangerState();
}

bool Player::hasAtClient(WorldObject* object) const
{
	return (object == this || m_clientGuids.find(object->getData()->getGuid()) != m_clientGuids.end());
}

void Player::removeFromClient(WorldObject* object)
{
	m_clientGuids.erase(object->getData()->getGuid());
}

void Player::cleanupAfterUpdate()
{
	this->getData()->clearUnitFlag(UNIT_FLAG_DAMAGED);

	Unit::cleanupAfterUpdate();
}

void Player::sendCreateClientObjectsForPlayer()
{
	if (!this->getSession())
		return;

	std::list<WorldObject*> objects;
	UpdateObject update;
	for (auto it = m_clientGuids.begin(); it != m_clientGuids.end(); ++it)
	{
		WorldObject* target = ObjectAccessor::getWorldObject(this, *it);
		if (target)
		{
			target->buildCreateUpdateBlockForPlayer(this, &update);
			objects.push_back(target);
		}
	}

	if (!update.hasUpdate())
		return;

	WorldPacket packet(SMSG_UPDATE_OBJECT);
	this->getSession()->packAndSend(std::move(packet), update);

	for (auto it = objects.begin(); it != objects.end(); ++it)
		(*it)->sendInitialVisiblePacketsToPlayer(this);
}

void Player::sendCreateClientLocatorsForPlayer()
{
	if (!this->getSession())
		return;

	UpdateObject update;
	for (auto it = m_clientTrackingGuids.begin(); it != m_clientTrackingGuids.end(); ++it)
	{
		WorldObject* target = ObjectAccessor::getWorldObject(this, *it);
		if(target && target->getLocator())
			target->getLocator()->buildCreateUpdateBlockForPlayer(this, &update);
	}

	if (!update.hasUpdate())
		return;

	WorldPacket packet(SMSG_UPDATE_OBJECT);
	this->getSession()->packAndSend(std::move(packet), update);
}

void Player::sendBattleResult(BattleOutcome outcome, int32 rankNo)
{
	if (!this->getSession())
		return;

	BattleResult result;
	result.set_outcome(outcome);
	result.set_kill_count(this->getData()->getKillCount());
	result.set_rank_no(rankNo);
	result.set_money(this->getData()->getMoney());

	int32 extraMoney = 0;
	int32 extraXP = 0;
	int32 giveMoney;
	int32 giveXP;

	this->calcRewardForRanking(rankNo, this->getMap()->getRankTotal(), &giveMoney, &giveXP);
	extraMoney += giveMoney;
	extraXP += giveXP;

	if (extraMoney > 0)
		result.set_extra_money(extraMoney);
	if (extraXP > 0)
		result.set_extra_xp(extraXP);

	this->addExperience(extraXP);
	result.set_level(this->getData()->getLevel());
	result.set_experience(this->getData()->getExperience());

	WorldPacket packet(SMSG_BATTLE_RESULT);
	this->getSession()->packAndSend(std::move(packet), result);

	NS_LOG_INFO("behaviors.player", "Player(%s) battle result, outcome: %d, extra money: %d, extra xp: %d", this->getData()->getGuid().toString().c_str(), outcome, extraMoney, extraXP);
}

void Player::sendLaunchResult(uint32 attackInfoCounter, int32 status)
{
	if (!this->getSession())
		return;

	LaunchResult message;
	message.set_launcher(this->getData()->getGuid().getRawValue());
	message.set_status(status);
	message.set_attack_info_counter(attackInfoCounter);

	WorldPacket packet(SMSG_LAUNCH_RESULT);
	this->getSession()->packAndSend(std::move(packet), message);
}

void Player::updateItemPickupResultForPlayer()
{
	if (!this->getPickupTarget())
		return;

	this->clearUnitState(UNIT_STATE_FORBIDDING_PICKUP);

	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(this->getPickupTarget()->getData()->getItemId());
	NS_ASSERT(tmpl);
	PickupStatus status = this->canStoreItem(tmpl, this->getPickupTarget()->getData()->getCount(), nullptr);
	if (status != PICKUP_STATUS_OK)
		this->addUnitState(UNIT_STATE_FORBIDDING_PICKUP);

	this->sendItemPickupResult(status);
}

void Player::sendItemPickupResult(PickupStatus status)
{
	if (!this->getSession() || !this->getPickupTarget())
		return;

	ItemPickupResult message;
	message.set_status(status);
	if (status == PICKUP_STATUS_OK)
		message.set_remaining_time(m_pickupTimer.getRemainder());
	message.set_item(this->getPickupTarget()->getData()->getGuid().getRawValue());

	WorldPacket packet(SMSG_ITEM_PICKUP_RESULT);
	this->getSession()->packAndSend(std::move(packet), message);
}

void Player::sendItemCooldownListForPlayer()
{
	if (!this->getSession())
		return;

	ItemCooldownList message;
	m_itemCooldownProcesser->buildItemCooldownList(message);
	if (message.result_size() > 0)
	{
		WorldPacket packet(SMSG_ITEM_COOLDOWN_LIST);
		this->getSession()->packAndSend(std::move(packet), message);
	}
}

void Player::sendItemUseResult(ObjectGuid const& guid, ItemUseStatus status)
{
	if (!this->getSession())
		return;

	ItemUseResult message;
	message.set_status(status);
	message.set_item(guid.getRawValue());

	WorldPacket packet(SMSG_ITEM_USE_RESULT);
	this->getSession()->packAndSend(std::move(packet), message);
}

void Player::sendItemActionMessage(ItemActionMessage::ActionType actionType, ItemTemplate const* itemTemplate)
{
	if (!this->getSession())
		return;

	ItemActionMessage message;
	message.set_type(actionType);
	message.set_owner(this->getData()->getGuid().getRawValue());
	message.set_item_id(itemTemplate->id);

	WorldPacket packet(SMSG_ITEM_ACTION_MESSAGE);
	this->getSession()->packAndSend(std::move(packet), message);
}

void Player::sendItemReceivedMessage(ItemTemplate const* itemTemplate, int32 count)
{
	if(itemTemplate->itemClass == ITEM_CLASS_GOLD)
		this->sendRewardMoneyMessage(count);

	this->sendItemActionMessage(ItemActionMessage::ITEM_RECEIVED, itemTemplate);
}

void Player::sendRewardXPMessage(int32 xp)
{
	if (!this->getSession())
		return;

	RewardMessage message;
	message.set_type(RewardMessage::REWARD_XP);
	message.set_value(xp);

	WorldPacket packet(SMSG_REWARD_MESSAGE);
	this->getSession()->packAndSend(std::move(packet), message);
}

void Player::sendRewardMoneyMessage(int32 amount)
{
	if (!this->getSession())
		return;

	RewardMessage message;
	message.set_type(RewardMessage::REWARD_MONEY);
	message.set_value(amount);

	WorldPacket packet(SMSG_REWARD_MESSAGE);
	this->getSession()->packAndSend(std::move(packet), message);
}

std::string Player::getStatusInfo()
{
	std::ios state(nullptr);
	std::stringstream ss;
	state.copyfmt(ss);
	ss << "name: " << this->getData()->getName();
	ss << ", displayid: " << this->getData()->getDisplayId();
	ss << ", screen size: " << (int32)this->getData()->getViewport().width << "x" << (int32)this->getData()->getViewport().height;
	ss << ", level: " << (int32)this->getData()->getLevel();
	ss << ", xp: " << m_earnedXP << "/" << this->getData()->getExperience() << "/" << this->getData()->getNextLevelXP();
	ss << ", ranking: " << m_rankNo;
	ss << ", health: " << this->getData()->getHealth() << "/" << this->getData()->getMaxHealth();
	ss << ", attacks: " << m_normalAttackCount << "/" << m_continuousAttackCount << "/" << m_chargedAttackCount;
	ss << ", money: " << this->getData()->getMoney() << "/" << this->getData()->getProperty();
	ss << ", stat stages: ";
	ss << std::setfill('0');
	ss << std::setw(2) << (int32)this->getData()->getStatStage(STAT_MAX_HEALTH);
	ss << std::setw(2) << (int32)this->getData()->getStatStage(STAT_HEALTH_REGEN_RATE);
	ss << std::setw(2) << (int32)this->getData()->getStatStage(STAT_ATTACK_RANGE);
	ss << std::setw(2) << (int32)this->getData()->getStatStage(STAT_MOVE_SPEED);
	ss << std::setw(2) << (int32)this->getData()->getStatStage(STAT_MAX_STAMINA);
	ss << std::setw(2) << (int32)this->getData()->getStatStage(STAT_STAMINA_REGEN_RATE);
	ss << std::setw(2) << (int32)this->getData()->getStatStage(STAT_ATTACK_TAKES_STAMINA);
	ss << std::setw(2) << (int32)this->getData()->getStatStage(STAT_DAMAGE);
	ss.copyfmt(state);
	ss << ", combat power: " << this->getData()->getCombatPower();
	if (this->getMap())
		ss << ", combat grade: " << (int32)this->getMap()->getCombatGrade().grade;
	ss << ", controller: " << this->getData()->getControllerType();
	float hitRate = 0.f;
	if (m_projectionCount > 0)
		hitRate = (m_hitCount / (float)m_projectionCount);
	ss << ", hit rate: " << std::setprecision(3) << hitRate;
	ss << ", kills: " << this->getData()->getKillCount();
	ss << ", itemboxs: " << m_itemBoxCount;
	ss << ", magicbeans: " << m_magicBeanCount;
	ss << ", items: ";
	for (auto it = m_itemHistoryList.begin(); it != m_itemHistoryList.end(); ++it)
	{
		uint32 itemId = (*it).first;
		int32 count = (*it).second;
		ss << itemId;
		if(count > 1)
		  ss << ";" << count;
		if (std::next(it) != m_itemHistoryList.end())
			ss << " ";
	}
	if (this->getMap())
	{
		ss << ", mapid: " << this->getMap()->getMapData()->getId();
		ss << ", battle state: " << this->getMap()->getBattleState();
		ss << ", battle time: " << this->getMap()->getBattleStateElapsedTime() << "/" << this->getMap()->getDurationByBattleState(this->getMap()->getBattleState());
	}
	if (WorldSession* session = this->getSession())
	{
		if (Theater* theater = session->getTheater())
		{
			ss << ", theaterid: " << theater->getTheaterId();
			ss << ", battle count: " << theater->getBattleCount();
		}
	}
	ss << ", reward stage: " << (int32)this->getData()->getRewardStage();
	ss << ", dailyreward days: " << this->getData()->getDailyRewardDays();
	ss << ", newplayer: " << this->getData()->isNewPlayer();
	ss << ", trainee: " << this->getData()->isTrainee();
	if (WorldSession* session = this->getSession())
		ss << ", latency: " << session->getLatency(LATENCY_MIN) << "/" << session->getLatency(LATENCY_MAX) << "/" << session->getLatency(LATENCY_AVG) << "ms";

	return ss.str();
}

void Player::concealed()
{
	NS_ASSERT(this->getData()->getConcealmentState() != CONCEALMENT_STATE_CONCEALED);

	Unit::concealed();
}

void Player::attack(float direction, uint32 attackInfoCounter)
{
	SimpleProjectile simpleProj;
	simpleProj.launcher = this->getData()->getGuid();
	simpleProj.launcherOrigin = this->getData()->getPosition();
	simpleProj.attackRange = this->getData()->getAttackRange();
	simpleProj.launchCenter = this->getData()->getLaunchCenter();
	simpleProj.launchRadiusInMap = this->getData()->getLaunchRadiusInMap();
	simpleProj.orientation = direction;
	simpleProj.chargedStamina = this->getData()->getChargedStamina();
	simpleProj.attackInfoCounter = attackInfoCounter;

	this->getData()->setOrientation(direction);

	// Projectile type
	simpleProj.projectileType = PROJECTILE_TYPE_NORMAL;
	if (this->hasUnitState(UNIT_STATE_CHARGING))
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

	// Damage bonus
	simpleProj.damageBonusRatio = 0.f;
	if ((simpleProj.projectileType == PROJECTILE_TYPE_CHARGED || simpleProj.projectileType == PROJECTILE_TYPE_INTENSIFIED) && this->hasItemEffectType(ITEM_EFFECT_DAMAGE_BONUS_PERCENT))
		simpleProj.damageBonusRatio = (float)getTotalItemEffectModifier(ITEM_EFFECT_DAMAGE_BONUS_PERCENT) / 100.f;

	// Calculate the size scale of the projectile
	simpleProj.scale = this->calcProjectileScale(simpleProj.projectileType);
	NS_ASSERT(simpleProj.scale >= 1.0f);

	// Calculate the stamina consumed
	int32 consumedStamina = this->calcConsumedStamina();
	simpleProj.consumedStamina = consumedStamina;
	this->getData()->increaseConsumedStaminaTotal(consumedStamina);
	this->getData()->increaseAttackCounter();
	simpleProj.attackCounter = this->getData()->getAttackCounter();

	// Calculate new stamina
	int32 newStamina = this->getData()->getStamina() - consumedStamina;
	NS_ASSERT(newStamina >= 0);
	this->getData()->setStamina(newStamina);

	//NS_LOG_DEBUG("behaviors.player", "ATTACK guid: %s direction: %f stamina: %d consumedStamina: %d charging: %d attackCounter: %d consumedStaminaTotal: %d isInfiniteStamina: %d", this->getData()->getGuid().toString().c_str(), direction, newStamina, consumedStamina, this->hasUnitState(UNIT_STATE_CHARGING), this->getData()->getAttackCounter(), this->getData()->getConsumedStaminaTotal(), this->getData()->isInfiniteStamina());

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

	m_projectionCount++;
}

void Player::updateAttackTimer(NSTime diff)
{
	if (!m_isContinuousAttack)
		return;

	m_attackTimer.update(diff);
	if (m_attackTimer.passed())
	{
		m_attackTimer.reset();
		--m_numAttacks;
		if (m_numAttacks <= 0
			|| this->getData()->getStamina() - this->getData()->getAttackTakesStamina() < this->getData()->getAttackTakesStamina())
		{
			this->attack(m_attackDirection, this->getData()->getAttackInfoCounter());
			m_numAttacks = 0;
			m_isContinuousAttack = false;
			m_attackDirection = 0;
		}
		else
			this->attack(m_attackDirection, 0);
	}
}

void Player::resetAttackTimer()
{
	m_attackTimer.reset();
}

void Player::updateUnlockerTimer(NSTime diff)
{
	if (!this->isInUnlock())
		return;

	m_unlockerTimer.update(diff);
	if (m_unlockerTimer.passed() || !m_unlocking)
	{
		this->unlockStop();
		m_unlockerTimer.reset();
	}
}

void Player::resetUnlockerTimer()
{
	m_unlockerTimer.setDuration(UNLOCKER_EXPIRATION_TIME);
}

void Player::calcRewardForRanking(int32 rankNo, int32 rankTotal, int32* giveMoney, int32* giveXP)
{
	int32 money = 0;
	int32 xp = 0;
	RewardForRanking const* reward = sObjectMgr->getRewardForRanking();
	if (reward)
	{
		money = reward->getMoney(rankNo, rankTotal);
		xp = reward->getXP(rankNo, rankTotal);
	}

	if (giveMoney)
		*giveMoney = money;
	if (giveXP)
		*giveXP = xp;
}

bool Player::updatePosition(Point const& newPosition)
{
	bool relocated = Unit::updatePosition(newPosition);
	if (relocated)
	{
		MapData* mapData = m_map->getMapData();
		TileCoord oldCoord(mapData->getMapSize(), this->getData()->getPosition());
		TileCoord newCoord(mapData->getMapSize(), newPosition);

		m_map->playerRelocation(this, newPosition);
		this->relocationTellAttackers();
		if (oldCoord != newCoord)
		{
			m_unitHostileRefManager.updateThreat(UNITTHREAT_DISTANCE);
			this->updateConcealmentState(newCoord);
		}
	}

	return relocated;
}

void Player::stopMoving()
{
	this->getData()->clearMovementFlag(MOVEMENT_FLAG_WALKING);
	this->getData()->markMoveSegment(this->getData()->getMovementInfo());
	this->clearUnitState(UNIT_STATE_MOVING);

	Unit::stopMoving();
}

void Player::transport(Point const& dest)
{
	this->combatStop();
	this->stopMoving();
	this->pickupStop();
	this->unlockStop();

	this->invisibleForNearbyPlayers();

	if (this->isAlive())
	{
		this->expose();
		this->concealIfNeeded(dest);
	}

	// Set a new position
	this->getMap()->playerRelocation(this, dest);
	this->getData()->increaseMovementCounter();
	this->getData()->markMoveSegment(this->getData()->getMovementInfo());
	m_unitHostileRefManager.updateThreat(UNITTHREAT_DISTANCE);

	if (this->getLocator())
	{
		this->getLocator()->invisibleForNearbyPlayers();
		this->getLocator()->getData()->setPosition(dest);
	}

	if (this->getSession())
	{
		WorldPacket packet(SMSG_TRANSPORT);
		this->getSession()->packAndSend(std::move(packet), this->getData()->getMovementInfo());
	}

	this->updateVisibilityInClient();
	this->updateObjectVisibility();
	this->updateTraceabilityInClient();
	this->updateObjectTraceability();
	this->updateObjectSafety();
}

void Player::addToWorld()
{
	if (this->isInWorld())
		return;

	Unit::addToWorld();

	ObjectAccessor::addObject(this);

	this->getMap()->increaseAliveCounter();
	this->getMap()->increasePlayerInBattleCounter();

	this->concealIfNeeded(this->getData()->getPosition());
}

void Player::removeFromWorld()
{
	if (!this->isInWorld())
		return;

	this->stopMoving();

	if (this->isAlive())
		this->getMap()->decreaseAliveCounter();

	if (this->getWithdrawalState() != WITHDRAWAL_STATE_WITHDREW)
		this->getMap()->decreasePlayerInBattleCounter();

	ObjectAccessor::removeObject(this);

	Unit::removeFromWorld();
}

void Player::sendInitSelf()
{
	if (!this->getSession())
		return;

	UpdateObject update;

	for (int32 i = 0; i < UNIT_SLOTS_COUNT; ++i)
	{
		CarriedItem* item = m_carriedItems[i];
		if (!item)
			continue;

		item->buildCreateUpdateBlockForPlayer(this, &update);
	}
	this->buildCreateUpdateBlockForPlayer(this, &update);

	WorldPacket packet(SMSG_UPDATE_OBJECT);
	this->getSession()->packAndSend(std::move(packet), update);

	this->sendInitialVisiblePacketsToPlayer(this);
}

void Player::sendInitialPacketsAfterAddToMap()
{
	this->updateVisibilityForPlayer();
	this->updateTraceabilityForPlayer();
}

void Player::sendResetPackets()
{
	this->sendItemCooldownListForPlayer();

	this->sendInitSelf();

	this->sendCreateClientObjectsForPlayer();
	this->sendCreateClientLocatorsForPlayer();
	this->updateItemPickupResultForPlayer();
}

void Player::spawn(BattleMap* map)
{
	PlayerTemplate const* tmpl = sObjectMgr->getPlayerTemplate(m_templateId);
	NS_ASSERT(tmpl);

	// Spawning position and orientation
	TileCoord spawnPoint = map->generateUnitSpawnPoint();
	this->getData()->setSpawnPoint(spawnPoint);
	Point position = spawnPoint.computePosition(map->getMapData()->getMapSize());
	this->getData()->increaseMovementCounter();
	this->getData()->setPosition(position);
	this->getData()->randomOrientation();
	this->getData()->markMoveSegment(this->getData()->getMovementInfo());

	// Initialize the locator
	if (this->getLocator())
	{
		this->getLocator()->getData()->setGuid(ObjectGuid(GUIDTYPE_UNIT_LOCATOR, map->generateUnitLocatorSpawnId()));
		this->getLocator()->getData()->setPosition(position);
		this->getLocator()->getData()->setAlive(true);
	}

	this->getData()->increaseStaminaCounter();

	this->resetAllStatModifiers();
	this->resetConcealmentTimer();
	this->resetWithdrawalTimer();
	this->resetDangerStateTimer();
	this->resetUnsaySmileyTimer();
	this->resetUnlockerTimer();
	this->getAI()->resetAI();
}

void Player::died(Unit* killer)
{
	Unit* champion;
	m_rewardManager.awardAllAwardees(&champion);

	std::vector<LootItem> itemList;
	this->fillLoot(killer, itemList);
	if (!itemList.empty())
	{
		int32 spentMoney;
		this->dropLoot(killer, itemList, &spentMoney);
		if (spentMoney > 0)
			this->sendRewardMoneyMessage(-spentMoney);
	}

	if(this->getLocator())
		this->getLocator()->getData()->setAlive(false);

	this->getData()->resetAttackCounter();
	this->getData()->resetConsumedStaminaTotal();

	this->setDeathState(DEATH_STATE_DEAD);
	m_map->playerKilled(this);

	if (!m_map->isTrainingGround())
		this->sendDeathMessageToAllPlayers(killer);
}

void Player::resetStats()
{
	PlayerTemplate const* tmpl = sObjectMgr->getPlayerTemplate(m_templateId);
	NS_ASSERT(tmpl);

	DataPlayer* data = this->getData();

	auto maxHealth = tmpl->getStageStats(data->getStatStage(STAT_MAX_HEALTH)).maxHealth;
	data->setMaxHealth(maxHealth);
	data->setBaseMaxHealth(maxHealth);
	data->setHealth(maxHealth);
	auto healthRegenRate = tmpl->getStageStats(data->getStatStage(STAT_HEALTH_REGEN_RATE)).healthRegenRate;
	data->setHealthRegenRate(healthRegenRate);
	data->setBaseHealthRegenRate(healthRegenRate);
	auto moveSpeed = tmpl->getStageStats(data->getStatStage(STAT_MOVE_SPEED)).moveSpeed;
	data->setMoveSpeed(moveSpeed);
	data->setBaseMoveSpeed(moveSpeed);
	auto attackRange = tmpl->getStageStats(data->getStatStage(STAT_ATTACK_RANGE)).attackRange;
	data->setAttackRange(attackRange);
	data->setBaseAttackRange(attackRange);
	int32 maxStamina = tmpl->getStageStats(data->getStatStage(STAT_MAX_STAMINA)).maxStamina;
	data->setMaxStamina(maxStamina);
	data->setStamina(maxStamina);
	data->setBaseMaxStamina(maxStamina);
	auto staminaRegenRate = tmpl->getStageStats(data->getStatStage(STAT_STAMINA_REGEN_RATE)).staminaRegenRate;
	data->setStaminaRegenRate(staminaRegenRate);
	data->setBaseStaminaRegenRate(staminaRegenRate);
	auto attackTakesStamina = tmpl->getStageStats(data->getStatStage(STAT_ATTACK_TAKES_STAMINA)).attackTakesStamina;
	data->setAttackTakesStamina(attackTakesStamina);
	data->setBaseAttackTakesStamina(attackTakesStamina);
	auto chargeConsumesStamina = tmpl->getStageStats(data->getStatStage(STAT_CHARGE_CONSUMES_STAMINA)).chargeConsumesStamina;
	data->setChargeConsumesStamina(chargeConsumesStamina);
	data->setBaseChargeConsumesStamina(chargeConsumesStamina);
	auto damage = tmpl->getStageStats(data->getStatStage(STAT_DAMAGE)).damage;
	data->setDamage(damage);
	data->setBaseDamage(damage);
	data->setDefense(0);
	data->setCombatPower(sObjectMgr->calcUnitCombatPower(data));

	this->resetAllStatModifiers();

	if(this->getLocator())
		this->getLocator()->getData()->setMoveSpeed(data->getMoveSpeed());
}

void Player::kill(Unit* victim)
{
	victim->died(this);

	int32 killCount = this->getData()->getKillCount();
	this->getData()->setKillCount(killCount + 1);
}

void Player::kill(ItemBox* itemBox)
{
	itemBox->opened(this);
	++m_itemBoxCount;
}

void Player::setLevelAndXP(uint8 level, int32 xp)
{
	PlayerTemplate const* tmpl = sObjectMgr->getPlayerTemplate(m_templateId);
	NS_ASSERT(tmpl);

	if (level < tmpl->originLevel)
		this->getData()->setLevel(tmpl->originLevel);
	else
		this->getData()->setLevel(std::min((uint8)LEVEL_MAX, level));

	this->getData()->setExperience(0);
	if (this->getData()->getLevel() < LEVEL_MAX)
		this->getData()->setNextLevelXP(assertNotNull(sObjectMgr->getUnitLevelXP(this->getData()->getLevel() + 1))->experience);
	else
		this->getData()->setNextLevelXP(0);

	if(xp > 0)
		this->addExperience(xp);
}

void Player::withdraw(BattleOutcome outcome, int32 rankNo)
{
	this->getMap()->decreasePlayerInBattleCounter();
	this->sendBattleResult(outcome, rankNo);
}

void Player::giveXP(int32 xp)
{
	int32 realXP = this->addExperience(xp);
	if (realXP != 0)
		this->sendRewardXPMessage(realXP);

	m_earnedXP += xp;
}

void Player::giveMoney(int32 money)
{
	if (money <= 0)
		return;

	int32 realAmount = this->modifyMoney(money);
	if (realAmount)
		this->sendRewardMoneyMessage(realAmount);
}

int32 Player::modifyMagicBean(ItemTemplate const* tmpl, int32 amount)
{
	int32 realAmount = Unit::modifyMagicBean(tmpl, amount);
	m_magicBeanCount = this->getData()->getMagicBeanCount();

	return realAmount;
}

bool Player::pickup(Item* supply)
{
	if (!Unit::pickup(supply))
		return false;

	this->getData()->setPickupTarget(supply->getData()->getGuid());
	this->updateItemPickupResultForPlayer();

	return true;
}

bool Player::pickupStop()
{
	if (!this->getPickupTarget())
		return false;

	this->clearUnitState(UNIT_STATE_FORBIDDING_PICKUP);
	this->getData()->setPickupTarget(ObjectGuid::EMPTY);

	return Unit::pickupStop();
}

bool Player::canPickup(Item* supply) const
{
	if (!Unit::canPickup(supply))
		return false;

	if (this->getData()->isGM())
		return false;

	return true;
}

void Player::receiveItem(Item* supply)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(supply->getData()->getItemId());
	NS_ASSERT(tmpl);

	ItemDest dest;
	if (this->canStoreItem(tmpl, supply->getData()->getCount(), &dest) != PICKUP_STATUS_OK)
		return;

	supply->receiveBy(this);

	this->storeItem(tmpl, dest);
	this->sendItemReceivedMessage(tmpl, dest.count);
}

void Player::removeItem(CarriedItem* item)
{
	this->getData()->setItem(item->getData()->getSlot(), ObjectGuid::EMPTY);
	item->sendDestroyToPlayer(this);

	Unit::removeItem(item);
}

CarriedItem* Player::createItem(ItemTemplate const* itemTemplate, ItemDest const& dest)
{
	CarriedItem* newItem = Unit::createItem(itemTemplate, dest);
	this->getData()->setItem(dest.pos, newItem->getData()->getGuid());
	newItem->sendCreateUpdateToPlayer(this);

	m_itemHistoryList.emplace_back(std::make_pair(newItem->getData()->getItemId(), newItem->getData()->getCount()));

	return newItem;
}

int32 Player::increaseItemCount(CarriedItem* item, int32 count)
{
	int32 newCount = Unit::increaseItemCount(item, count);
	m_itemHistoryList.emplace_back(std::make_pair(item->getData()->getItemId(), count));

	return newCount;
}

int32 Player::decreaseItemCount(CarriedItem* item)
{
	int32 newCount = Unit::decreaseItemCount(item);
	if(newCount > 0)
		item->sendValuesUpdateToPlayer(this);

	return newCount;
}

void Player::useItem(CarriedItem* item)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item->getData()->getItemId());
	NS_ASSERT(tmpl);

	m_itemCooldownProcesser->startCooldown(item);

	this->sendItemUseResult(item->getData()->getGuid(), ITEM_USE_STATUS_OK);
	m_itemApplicationProcesser->applyItem(tmpl);

	this->takeItem(item);
}

void Player::takeItem(CarriedItem* item)
{
	Unit::takeItem(item);

	if (this->hasUnitState(UNIT_STATE_FORBIDDING_PICKUP) && this->getPickupTarget())
	{
		this->pickupStop();
		this->updateObjectVisibility();
	}
}

void Player::setSession(WorldSession* session)
{
	if (m_session != session)
	{
		m_session = session;
	}
}

void Player::discardSession()
{
	if (m_session)
	{
		this->getData()->increaseMovementCounter();
		this->getData()->increaseStaminaCounter();
	}

	m_session = nullptr;
}