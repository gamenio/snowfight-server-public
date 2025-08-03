#include "Unit.h"

#include "game/server/protocol/pb/DeathMessage.pb.h"

#include "utilities/TimeUtil.h"
#include "game/utils/MathTools.h"
#include "game/grids/ObjectSearcher.h"
#include "game/world/ObjectMgr.h"
#include "game/world/World.h"
#include "game/ai/UnitAI.h"
#include "game/maps/BattleMap.h"
#include "game/maps/MapDataManager.h"
#include "Robot.h"
#include "Item.h"
#include "Player.h"
#include "ItemBox.h"
#include "UnitHelper.h"
#include "Projectile.h"
#include "UnitLocator.h"

#define HEALTH_REGEN_BASE_INTERVAL								5000	// 生命值恢复的基本间隔时间
#define HEALTH_REGEN_RATE_PERCENT_WHEN_CONCEALED				150		// 隐蔽时增加生命值恢复速度百分比
#define SMILEY_DURATION											5000	// 表情持续时间

#define SLOW_MOVE_DURATION										500		// 当受到伤害时减慢移动的持续时间
#define MOVE_SPEED_PERCENT_WHEN_SLOW_MOVE						-60		// 缓慢移动时降低移动速度百分比

#define PROJECTILE_SCALE_TO_STAMINA_RATIO						0.02f	// 抛射体大小比例与体力值的比率。单位：抛射体大小比例/每点体力
#define INTENSIFIED_PROJECTILE_SCALE							1.5f	// 强化的抛射体大小比例

#define MAGICBEAN_DROP_REFERENCE_CHANCE							23.f	// 魔豆和单位数为1比1的情况下单位掉落魔豆的几率

Unit::Unit() :
	m_unitState(UnitState::UNIT_STATE_NONE),
	m_deathState(DeathState::DEATH_STATE_ALIVE),
	m_dangerState(DangerState::DANGER_STATE_RELEASED),
	m_healthRegenInterval(HEALTH_REGEN_BASE_INTERVAL),
	m_rewardManager(this),
	m_battleOutcome(BATTLE_VICTORY),
	m_rankNo(0),
	m_withdrawalState(WITHDRAWAL_STATE_NONE),
	m_unlocking(nullptr),
	m_pickingUp(nullptr),
	m_itemApplicationProcesser(new ItemApplicationProcesser(this)),
	m_itemCooldownProcesser(new ItemCooldownProcesser(this)),
	m_carriedItemSpawnIdCounter(0),
	m_ai(nullptr),
	m_isSlowMoveEnabled(false)
{

	m_type |= TypeMask::TYPEMASK_UNIT;
	m_typeId = TypeID::TYPEID_UNIT;

	std::fill_n(m_carriedItems, (int32)UNIT_SLOTS_COUNT, nullptr);
}


Unit::~Unit()
{
	for (int32 i = 0; i < UNIT_SLOTS_COUNT; ++i)
	{
		if (m_carriedItems[i])
		{
			delete m_carriedItems[i];
			m_carriedItems[i] = nullptr;
		}
	}

	if (m_itemApplicationProcesser)
	{
		delete m_itemApplicationProcesser;
		m_itemApplicationProcesser = nullptr;
	}

	if (m_itemCooldownProcesser)
	{
		delete m_itemCooldownProcesser;
		m_itemCooldownProcesser = nullptr;
	}
}

void Unit::setDeathState(DeathState state)
{
	m_deathState = state;

	switch (state)
	{
	case DEATH_STATE_ALIVE:
		this->normalizeHealth();
		this->normalizeHealthRegenRate();
		this->normalizeStamina();
		this->normalizeAttackRange();
		this->normalizeMoveSpeed();
		this->normalizeDamage();
		break;
	case DEATH_STATE_DEAD:
		this->combatStop();
		this->stopMoving();
		this->pickupStop();
		this->unlockStop();
		this->stopStaminaUpdate();

		this->removeAllAttackers();
		this->removeAllEnemies();
		m_followerRefManager.clearReferences();
		m_unitHostileRefManager.clearReferences();
		m_observerRefManager.clearReferences();
		m_awardSourceManager.clearReferences();

		this->removeAllItems();
		m_itemCooldownProcesser->removeAll();
		m_itemApplicationProcesser->removeAll();

		this->getData()->setMagicBeanCount(0);
		this->getData()->setHealth(0);
		this->getData()->setStamina(0);
		this->expose();
		this->releaseDangerState();
		this->resetHealthRegenTimer();
		this->resetHealthLossTimer();
		break;
	}
}

void Unit::sendDeathMessageToAllPlayers(Unit* killer)
{
	auto const& playerList = this->getMap()->getPlayerList();
	for (auto it = playerList.begin(); it != playerList.end(); ++it)
	{
		Player* player = (*it).second;
		if (!player->getSession())
			continue;

		WorldPacket packet(SMSG_DEATH_MESSAGE);
		DeathMessage message;
		message.set_time(player->getSession()->getClientNowTimeMillis());
		if (killer)
		{
			message.mutable_killer()->set_guid(killer->getData()->getGuid().getRawValue());
			message.mutable_killer()->set_name(killer->getData()->getName());
		}
		message.mutable_victim()->set_guid(this->getData()->getGuid().getRawValue());
		message.mutable_victim()->set_name(this->getData()->getName());
		player->getSession()->packAndSend(std::move(packet), message);
	}
}

void Unit::fillLoot(Unit* looter, std::vector<LootItem>& itemList)
{
	int32 magicBeanCount = this->getData()->getMagicBeanCount();
	if (magicBeanCount > 0)
	{
		ItemTemplate const* tmpl = sObjectMgr->getMagicBeanTemplate();
		NS_ASSERT(tmpl);

		uint32 totalMagicBeans = m_map->getSpawnManager()->getClassifiedItemCount(ITEM_CLASS_MAGIC_BEAN);
		// 计算每个魔豆增加的掉落几率
		float chanceInc = MAGICBEAN_DROP_REFERENCE_CHANCE / (totalMagicBeans / (float)m_map->getMapData()->getPopulationCap());
		LootItem item;
		item.itemId = tmpl->id;
		item.chance = std::min(100.f, magicBeanCount * chanceInc);
		item.minCount = 1;
		item.maxCount = 0;
		itemList.push_back(item);
	}

	// 最后活着的单位不会掉落金币
	if (this->getMap()->getAliveCount() > 1)
	{
		CombatGrade const& combatGrade = this->getMap()->getCombatGrade();
		int32 currMoney = this->getData()->getMoney();
		int32 loseMoney = (int32)(currMoney * (combatGrade.loseMoneyPercent / 100.f));
		if (loseMoney > 0)
		{
			ItemTemplate const* tmpl = sObjectMgr->getGoldTemplate();
			NS_ASSERT(tmpl);

			LootItem item;
			item.itemId = tmpl->id;
			item.chance = 100.f;
			item.minCount = loseMoney;
			item.maxCount = 0;
			itemList.push_back(item);
		}
	}

	for (int32 i = 0; i < UNIT_SLOTS_COUNT; ++i)
	{
		CarriedItem* carriedItem = m_carriedItems[i];
		if (!carriedItem)
			continue;

		ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(carriedItem->getData()->getItemId());
		NS_ASSERT(tmpl);

		UnitLootItemTemplate const* itemTmpl = sObjectMgr->getUnitLootItemTemplate(tmpl->itemClass, tmpl->itemSubClass);
		if (!itemTmpl)
			return;

		LootItem item;
		item.itemId = tmpl->id;
		item.chance = itemTmpl->chance;
		item.minCount = 1;
		item.maxCount = std::min(itemTmpl->maxCount, carriedItem->getData()->getCount());
		itemList.push_back(item);
	}
}

void Unit::dropLoot(Unit* looter, std::vector<LootItem> const& itemList, int32* spentMoney)
{
	if (spentMoney)
		*spentMoney = 0;

	// 初始化地面物品位置
	TileCoord currCoord(m_map->getMapData()->getMapSize(), this->getData()->getPosition());
	FloorItemPlace itemPlace(m_map, currCoord, FloorItemPlace::LEFT_DOWN);

	// 根据几率掉落物品
	int32 grade = std::max(0, (int32)(this->getMap()->getCombatGrade().grade - 1));
	for (auto it = itemList.begin(); it != itemList.end(); ++it)
	{
		LootItem const& lootItem = *it;
		if (rollChance(lootItem.chance))
		{
			int32 itemCount;
			if (lootItem.minCount < lootItem.maxCount)
				itemCount = random(lootItem.minCount, lootItem.maxCount);
			else
				itemCount = lootItem.minCount;
			ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(lootItem.itemId);
			NS_ASSERT(tmpl);
			if (tmpl->itemClass == ITEM_CLASS_MAGIC_BEAN)
			{
				int32 maxCount = MAGIC_BEAN_DROPPED_MAX_STACK_COUNT;
				int32 nStacks = itemCount / maxCount + ((itemCount % maxCount) ? 1 : 0);
				for (int32 i = 0; i < nStacks; ++i)
				{
					int32 count = std::min(itemCount, maxCount);
					this->createDroppedItem(tmpl, itemPlace.nextEmptyTile(), count);
					itemCount -= maxCount;
				}
			}
			else if (tmpl->itemClass == ITEM_CLASS_GOLD)
			{
				int32 currMoney = this->getData()->getMoney();
				int32 bounty = this->takeBounty(itemCount);
				if (bounty > 0)
				{
					if (spentMoney)
						*spentMoney = currMoney - this->getData()->getMoney();
					if (m_map->isShowdown())
					{
						if (looter)
						{
							looter->giveMoney(bounty);
							this->getData()->addUnitFlag(UNIT_FLAG_DEATH_LOSE_MONEY);
						}
					}
					else
						this->createDroppedItem(tmpl, itemPlace.nextEmptyTile(), itemCount);
				}
			}
			else
			{
				for (int32 i = 0; i < itemCount; ++i)
					this->createDroppedItem(tmpl, itemPlace.nextEmptyTile(), 1);
			}
		}
	}
}

void Unit::withdrawDelayed(BattleOutcome outcome, int32 rankNo)
{
	if (m_withdrawalState != WITHDRAWAL_STATE_NONE)
		return;

	m_battleOutcome = outcome;
	m_rankNo = rankNo;
	m_withdrawalTimer.reset();
	m_withdrawalState = WITHDRAWAL_STATE_WITHDRAWING;
}

void Unit::deleteAwardeeList()
{
	m_rewardManager.removeAllAwardees();
}

void Unit::addDamageOfAwardee(Unit* attacker, int32 damage)
{
	m_rewardManager.addDamage(attacker, damage);
}

float Unit::calcReward(Unit* attacker, int32 damagePoints)
{
	int32 aggDamage = m_rewardManager.getAggDamage();
	CombatGrade const& combatGrade = this->getMap()->getCombatGrade();

	// 对受害者造成的伤害
	float r1 = (float)damagePoints / aggDamage;
	float r2 = std::min(1.0f, aggDamage / (float)this->getData()->getMaxHealth());

	float reward = r1 * r2;

	return reward;
}

void Unit::applyReward(Unit* victim, float reward)
{
	if (this->getMap()->isTrainingGround())
		return;

	RewardOnKillUnit const* rewardData = sObjectMgr->getRewardOnKillUnit();
	NS_ASSERT(rewardData);

	// 计算经验值和等级
	int32 gainXP = (int32)std::round(rewardData->xp * reward);
	if (gainXP > 0)
		this->giveXP(gainXP);
}

void Unit::updateHealthRegenTimer(NSTime diff)
{
	int32 health = this->getData()->getHealth();
	int32 maxHealth = this->getData()->getMaxHealth();
	if (health >= maxHealth || m_dangerState == DANGER_STATE_ENTERED)
		return;

	m_healthRegenTimer.update(diff);
	if (m_healthRegenTimer.passed())
	{
		int32 points = (int32)(m_healthRegenTimer.getElapsed() / 60000.0f * this->getData()->getHealthRegenRate() * maxHealth);
		int32 newHealth = health + points;
		if (newHealth > maxHealth)
			this->getData()->setHealth(maxHealth);
		else
			this->getData()->setHealth(newHealth);
		this->resetHealthRegenTimer();
		this->getUnitHostileRefManager()->updateThreat(UNITTHREAT_ENEMY_HEALTH);
	}
}

void Unit::updateHealthLossTimer(NSTime diff)
{
	if (m_dangerState != DANGER_STATE_ENTERED)
		return;

	m_healthLossTimer.update(diff);
	if (m_healthLossTimer.passed())
	{
		int32 health = this->getData()->getHealth();
		int32 points = (int32)(m_healthLossTimer.getElapsed() / 1000.f * this->getMap()->getCombatGrade().dangerStateHealthLoss);
		if (health > points)
			this->receiveDamage(nullptr, points);
		else
			this->died(nullptr);
		this->resetHealthLossTimer();
	}
}

void Unit::resetHealthRegenTimer()
{
	m_healthRegenTimer.setDuration(m_healthRegenInterval);
}

void Unit::resetHealthLossTimer()
{
	m_healthLossTimer.setDuration(sWorld->getIntConfig(CONFIG_DANGER_STATE_HEALTH_LOSS_INTERVAL));
}

float Unit::calcProjectileScale(ProjectileType projType)
{
	float scale = 1.0f;
	if (projType == PROJECTILE_TYPE_CHARGED)
	{
		int32 chargedStamina = this->getData()->getChargedStamina();
		if (chargedStamina > this->getData()->getAttackTakesStamina())
			scale = 1.0f + (chargedStamina - this->getData()->getAttackTakesStamina()) * PROJECTILE_SCALE_TO_STAMINA_RATIO;
	}
	else if (projType == PROJECTILE_TYPE_INTENSIFIED)
		scale = INTENSIFIED_PROJECTILE_SCALE;

	return scale;
}

void Unit::resetWithdrawalTimer()
{
	m_withdrawalTimer.setDuration(sWorld->getIntConfig(CONFIG_WITHDRAWAL_DELAY));
}

void Unit::updateWithdrawalTimer(NSTime diff)
{
	if (m_withdrawalState != WITHDRAWAL_STATE_WITHDRAWING)
		return;

	m_withdrawalTimer.update(diff);
	if (m_withdrawalTimer.passed())
	{
		this->withdraw(m_battleOutcome, m_rankNo);

		m_withdrawalTimer.reset();
		m_withdrawalState = WITHDRAWAL_STATE_WITHDREW;
	}
}

void Unit::updateUnsaySmileyTimer(NSTime diff)
{
	if (this->getData()->getSmiley() == SMILEY_NONE)
		return;

	m_unsaySmileyTimer.update(diff);
	if (m_unsaySmileyTimer.passed())
	{
		this->getData()->setSmiley(SMILEY_NONE);
		m_unsaySmileyTimer.reset();
	}
}

void Unit::resetUnsaySmileyTimer()
{
	m_unsaySmileyTimer.setDuration(SMILEY_DURATION);
}

void Unit::unsaySmiley()
{
	this->getData()->setSmiley(SMILEY_NONE);
	this->resetUnsaySmileyTimer();
}

void Unit::updateConcealmentTimer(NSTime diff)
{
	if (this->getData()->getConcealmentState() != CONCEALMENT_STATE_CONCEALING)
		return;

	m_concealmentTimer.update(diff);
	if (m_concealmentTimer.passed())
	{
		m_concealmentTimer.reset();
		this->concealed();
	}
}

void Unit::resetConcealmentTimer()
{
	m_concealmentTimer.setDuration(sWorld->getIntConfig(CONFIG_CONCEALED_DELAY));
}

void Unit::concealed()
{
	MapData* mapData = m_map->getMapData();
	TileCoord currCoord(mapData->getMapSize(), this->getData()->getPosition());
	NS_ASSERT(mapData->isConcealable(currCoord), "The current tile is not concealed.");

	if (mapData->isConcealable(currCoord))
	{
		NS_ASSERT(this->getData()->getConcealmentState() != CONCEALMENT_STATE_CONCEALED);
		this->getData()->setConcealmentState(CONCEALMENT_STATE_CONCEALED);
		this->handleStatModifier(STAT_HEALTH_REGEN_RATE, STAT_MODIFIER_PERCENT, HEALTH_REGEN_RATE_PERCENT_WHEN_CONCEALED, true);

		this->updateObjectVisibility();
	}
}

void Unit::updateDangerStateTimer(NSTime diff)
{
	if (m_dangerState != DANGER_STATE_ENTERING)
		return;

	m_dangerStateTimer.update(diff);
	if (m_dangerStateTimer.passed())
	{
		m_dangerStateTimer.reset();
		m_dangerState = DANGER_STATE_ENTERED;

		this->resetHealthRegenTimer();
		this->resetHealthLossTimer();
		m_healthLossTimer.setPassed();
	}
}

void Unit::resetDangerStateTimer()
{
	m_dangerStateTimer.setDuration(sWorld->getIntConfig(CONFIG_ENTERING_DANGER_STATE_DELAY));
}

void Unit::slowMoveSpeed(NSTime duration)
{
	if (m_isSlowMoveEnabled)
		this->restoreMoveSpeed();

	this->handleStatModifier(STAT_MOVE_SPEED, STAT_MODIFIER_PERCENT, MOVE_SPEED_PERCENT_WHEN_SLOW_MOVE, true);

	m_slowMoveTimer.setDuration(duration);
	m_isSlowMoveEnabled = true;
}

void Unit::updateSlowMoveTimer(NSTime diff)
{
	if (!m_isSlowMoveEnabled)
		return;

	m_slowMoveTimer.update(diff);
	if (m_slowMoveTimer.passed())
	{
		this->restoreMoveSpeed();
	}
}

void Unit::restoreMoveSpeed()
{
	if (!m_isSlowMoveEnabled)
		return;

	this->handleStatModifier(STAT_MOVE_SPEED, STAT_MODIFIER_PERCENT, MOVE_SPEED_PERCENT_WHEN_SLOW_MOVE, false);

	m_slowMoveTimer.reset();
	m_isSlowMoveEnabled = false;
}

void Unit::updatePickupTimer(NSTime diff)
{
	if (!m_pickingUp || this->hasUnitState(UNIT_STATE_FORBIDDING_PICKUP))
		return;

	m_pickupTimer.update(diff);
	if (m_pickupTimer.passed())
	{
		this->receiveItem(m_pickingUp);
		this->pickupStop();
	}
}

PickupStatus Unit::canStoreItemInInventoryCustomSlots(ItemTemplate const* itemTemplate, int32 count, ItemDest* dest) const
{
	PickupStatus status = PICKUP_STATUS_INVENTORY_CUSTOM_SLOTS_FULL;
	int32 pos = SLOT_INVALID;
	for (int32 i = INVENTORY_SLOT_CUSTOM_START; i < INVENTORY_SLOT_CUSTOM_END; ++i)
	{
		CarriedItem* item = m_carriedItems[i];
		if (item)
		{
			if (item->getData()->getItemId() == itemTemplate->id)
			{
				status = item->canBeMergedPartlyWith(itemTemplate);
				if (status == PICKUP_STATUS_OK)
					pos = i;
				else
					pos = SLOT_INVALID;
				break;
			}
		}
		else
		{
			if (pos == SLOT_INVALID)
				pos = i;
		}
	}

	if (dest)
	{
		dest->pos = pos;
		int32 noSpaceCount = this->getCannotStoreItemCount(itemTemplate, pos, count);
		dest->count = count - noSpaceCount;
	}

	if (pos != SLOT_INVALID)
		return PICKUP_STATUS_OK;

	return status;
}

PickupStatus Unit::canStoreStackableItemInSpecificSlot(int32 slot, ItemTemplate const* itemTemplate, int32 count, ItemDest* dest) const
{
	NS_ASSERT(itemTemplate->stackable != ITEM_STACK_NON_STACKABLE);

	int32 pos = SLOT_INVALID;
	PickupStatus status;
	CarriedItem* existingItem = m_carriedItems[slot];
	if (!existingItem)
		status = PICKUP_STATUS_OK;
	else
		status = existingItem->canBeMergedPartlyWith(itemTemplate);
	if (status == PICKUP_STATUS_OK)
		pos = slot;

	if (dest)
	{
		dest->pos = pos;
		int32 noSpaceCount = this->getCannotStoreItemCount(itemTemplate, pos, count);
		dest->count = count - noSpaceCount;
	}

	return status;
}

PickupStatus Unit::canStoreItemInEquipmentSlot(ItemTemplate const* itemTemplate, ItemDest* dest) const
{
	PickupStatus status = PICKUP_STATUS_FORBIDDEN;
	int32 equipSlot = SLOT_INVALID;
	switch (itemTemplate->itemSubClass)
	{
	case ITEM_SUBCLASS_HAT:
		equipSlot = EQUIPMENT_SLOT_HAT;
		break;
	case ITEM_SUBCLASS_JACKET:
		equipSlot = EQUIPMENT_SLOT_JACKET;
		break;
	case ITEM_SUBCLASS_GLOVES:
		equipSlot = EQUIPMENT_SLOT_GLOVES;
		break;
	case ITEM_SUBCLASS_SNOWBALL_MAKER:
		equipSlot = EQUIPMENT_SLOT_SNOWBALL_MAKER;
		break;
	case ITEM_SUBCLASS_SHOES:
		equipSlot = EQUIPMENT_SLOT_SHOES;
		break;
	default:
		break;
	}

	int32 pos = SLOT_INVALID;
	if (equipSlot != SLOT_INVALID)
	{
		CarriedItem* existingItem = m_carriedItems[equipSlot];
		if (!existingItem)
			status = PICKUP_STATUS_OK;
		else
		{
			if (existingItem->getData()->getItemId() == itemTemplate->id)
				status = PICKUP_STATUS_ITEM_IS_EQUIPPED;
			else if (itemTemplate->level > 0 && existingItem->getData()->getLevel() > itemTemplate->level)
				status = PICKUP_STATUS_LEVEL_LOWER_THAN_EXISTING_EQUIP;
			else
				status = PICKUP_STATUS_OK;
		}
		if (status == PICKUP_STATUS_OK)
			pos = equipSlot;
	}

	if (dest)
	{
		dest->pos = pos;
		dest->count = pos == SLOT_INVALID ? 0 : 1;
	}

	return status;
}

int32 Unit::getCannotStoreItemCount(ItemTemplate const* itemTemplate, int32 itemPos, int32 count) const
{
	if (itemPos == SLOT_INVALID)
		return count;

	int32 maxCount = itemTemplate->stackable == ITEM_STACK_NON_STACKABLE ? 1 : itemTemplate->stackable;
	int32 noSpaceCount = 0;
	if (maxCount > 0)
	{
		CarriedItem* item = m_carriedItems[itemPos];
		if (item)
			count += item->getData()->getCount();

		if (count > maxCount)
			noSpaceCount = count - maxCount;
	}

	return noSpaceCount;
}

int32 Unit::addExperience(int32 xp)
{
	int32 realXP = 0;
	uint8 currLevel = this->getData()->getLevel();
	if (currLevel < LEVEL_MAX)
	{
		uint8 newLevel = currLevel;
		int32 currXP = this->getData()->getExperience();
		int32 newXP = currXP + xp;
		realXP = xp;
		if (newXP < 0)
		{
			realXP -= newXP;
			newXP = 0;
		}
		int32 nextLevelXP = this->getData()->getNextLevelXP();
		while (newXP >= nextLevelXP)
		{
			newXP -= nextLevelXP;
			++newLevel;
			if (newLevel < LEVEL_MAX)
				nextLevelXP = assertNotNull(sObjectMgr->getUnitLevelXP(newLevel + 1))->experience;
			else
			{
				// 满级
				realXP -= newXP;
				newXP = 0;
				nextLevelXP = 0;
				break;
			}
		}

		this->getData()->setExperience(newXP);
		if (newLevel > currLevel)
		{
			this->getData()->setNextLevelXP(nextLevelXP);
			this->getData()->setLevel(newLevel);
		}
	}

	return realXP;
}

void Unit::createDroppedItem(ItemTemplate const* tmpl, TileCoord const& spawnPoint, int32 count)
{
	SimpleItem simpleItem;
	simpleItem.templateId = tmpl->id;
	simpleItem.count = count;
	simpleItem.holder = this->getData()->getGuid();
	simpleItem.holderOrigin = this->getData()->getPosition();
	simpleItem.spawnPoint = spawnPoint;
	simpleItem.launchCenter = this->getData()->getLaunchCenter();
	simpleItem.dropDuration = random(ITEM_DROP_DELAY_MIN, ITEM_DROP_DELAY_MAX) + ITEM_PARABOLA_DURATION;

	Item* item = m_map->takeReusableObject<Item>();
	if (item)
		item->reloadData(simpleItem, m_map);
	else
	{
		item = new Item();
		item->loadData(simpleItem, m_map);
	}
	m_map->addToMap(item);
	item->updateObjectVisibility(true);
}

int32 Unit::modifyMoney(int32 amount)
{
	if (!amount)
		return 0;

	int32 money = this->getData()->getMoney();
	if (amount < 0)
	{
		if (money > std::abs(amount))
			money += amount;
		else
		{
			amount = -money;
			money = 0;
		}
		this->getData()->setMoney(money);
	}
	else
		this->getData()->setMoney(money + amount);

	return amount;
}

int32 Unit::modifyMagicBean(ItemTemplate const* tmpl, int32 amount)
{
	if (!amount)
		return 0;

	int32 count = this->getData()->getMagicBeanCount();
	if (amount < 0)
	{
		if (count > std::abs(amount))
			count += amount;
		else
		{
			amount = -count;
			count = 0;
		}
		this->getData()->setMagicBeanCount(count);
		if(amount)
			this->applyItemStats(tmpl->itemStats, false, std::abs(amount));
	}
	else
	{
		this->getData()->setMagicBeanCount(count + amount);
		this->applyItemStats(tmpl->itemStats, true, amount);
	}

	return amount;
}

int32 Unit::takeBounty(int32 amount)
{
	float apportionmentRatio = this->getMap()->getCombatGrade().loseMoneyApportionment / 100.f;
	int32 currMoney = this->getData()->getMoney();
	int32 bounty = (int32)std::round((1.0f - apportionmentRatio) * amount);
	int32 apportionedAmount = amount - bounty;

	int32 spentMoney;
	if (apportionedAmount < currMoney)
		spentMoney = apportionedAmount;
	else
		spentMoney = currMoney;

	if (spentMoney > 0)
	{
		int32 newMoney = currMoney - spentMoney;
		NS_ASSERT(newMoney >= 0);
		this->getData()->setMoney(newMoney);

		bounty += spentMoney;
	}

	return bounty;
}

void Unit::saySmiley(uint16 code)
{
	this->getData()->setSmiley(code);
	if (code != SMILEY_NONE)
		this->resetUnsaySmileyTimer();
}

void Unit::expose()
{
	if (this->getData()->getConcealmentState() == CONCEALMENT_STATE_EXPOSED)
		return;

	switch (this->getData()->getConcealmentState())
	{
	case CONCEALMENT_STATE_CONCEALING:
		this->resetConcealmentTimer();
		break;
	case CONCEALMENT_STATE_CONCEALED:
		this->handleStatModifier(STAT_HEALTH_REGEN_RATE, STAT_MODIFIER_PERCENT, HEALTH_REGEN_RATE_PERCENT_WHEN_CONCEALED, false);
		this->updateObjectVisibility();
	default:
		break;
	}

	this->getData()->setConcealmentState(CONCEALMENT_STATE_EXPOSED);
}

void Unit::concealDelayed()
{
	if (this->getData()->getConcealmentState() != CONCEALMENT_STATE_EXPOSED)
		return;

	this->resetConcealmentTimer();
	this->getData()->setConcealmentState(CONCEALMENT_STATE_CONCEALING);
}

void Unit::concealIfNeeded(Point const& position)
{
	MapData* mapData = m_map->getMapData();
	TileCoord tileCoord(mapData->getMapSize(), position);
	if (mapData->isConcealable(tileCoord))
		this->concealDelayed();
}

void Unit::updateConcealmentState(TileCoord const& tileCoord)
{
	if (!this->isAlive())
		return;

	MapData* mapData = m_map->getMapData();
	if (mapData->isConcealable(tileCoord))
		this->concealDelayed();
	else
		this->expose();
}


void Unit::releaseDangerState()
{
	if (m_dangerState == DANGER_STATE_RELEASED)
		return;

	switch (m_dangerState)
	{
	case DANGER_STATE_ENTERING:
		this->resetDangerStateTimer();
		break;
	default:
		break;
	}
	m_dangerState = DANGER_STATE_RELEASED;
}

void Unit::enterDangerState()
{
	if (m_dangerState != DANGER_STATE_RELEASED)
		return;

	this->resetDangerStateTimer();
	m_dangerState = DANGER_STATE_ENTERING;
}

bool Unit::canPickup(Item* supply) const
{
	if (!this->isAlive() || !supply->isInWorld() || !supply->isVisible())
		return false;

	return true;
}

bool Unit::pickup(Item* supply)
{
	if (supply == m_pickingUp)
		return false;

	if (m_pickingUp)
	{
		float d1 = m_pickingUp->getData()->getPosition().getDistanceSquared(this->getData()->getPosition());
		float d2 = supply->getData()->getPosition().getDistanceSquared(this->getData()->getPosition());
		if (d2 > d1)
			return false;
	}

	this->addUnitState(UNIT_STATE_PICKING_UP);

	if (m_pickingUp)
		m_pickingUp->removePicker(this);
	supply->addPicker(this);

	m_pickingUp = supply;

	m_pickupTimer.setDuration(this->getData()->getPickupDuration());

	return true;
}

bool Unit::pickupStop()
{
	if (!m_pickingUp)
		return false;

	m_pickingUp->removePicker(this);
	m_pickingUp = nullptr;

	this->clearUnitState(UNIT_STATE_PICKING_UP);

	m_pickupTimer.reset();

	return true;
}

PickupStatus Unit::canStoreItem(ItemTemplate const* itemTemplate, int32 count, ItemDest* dest) const
{
	if (dest)
	{
		dest->pos = SLOT_INVALID;
		dest->count = 0;
	}
	PickupStatus status = PICKUP_STATUS_FORBIDDEN;
	switch (itemTemplate->itemClass)
	{
	case ITEM_CLASS_CONSUMABLE:
		if (itemTemplate->itemSubClass == ITEM_SUBCLASS_FIRST_AID)
			status = this->canStoreStackableItemInSpecificSlot(INVENTORY_SLOT_FIRST_AID, itemTemplate, count, dest);
		else if (itemTemplate->itemSubClass == ITEM_SUBCLASS_CONSUMABLE_OTHER)
			status = this->canStoreItemInInventoryCustomSlots(itemTemplate, count, dest);
		break;
	case ITEM_CLASS_EQUIPMENT:
		status = this->canStoreItemInEquipmentSlot(itemTemplate, dest);
		break;
	case ITEM_CLASS_GOLD:
	case ITEM_CLASS_MAGIC_BEAN:
		status = PICKUP_STATUS_OK;
		if (dest)
			dest->count = count;
		break;
	}

	return status;
}

void Unit::storeItem(ItemTemplate const* itemTemplate, ItemDest const& dest)
{
	if (itemTemplate->itemClass == ITEM_CLASS_GOLD)
		this->modifyMoney(dest.count);
	else if (itemTemplate->itemClass == ITEM_CLASS_MAGIC_BEAN)
		this->modifyMagicBean(itemTemplate, dest.count);
	else
	{
		CarriedItem* existingItem = m_carriedItems[dest.pos];
		if (existingItem)
		{
			switch (itemTemplate->itemClass)
			{
			case ITEM_CLASS_EQUIPMENT:
				existingItem->drop();
				this->removeItem(existingItem);
				existingItem = nullptr;
				break;
			case ITEM_CLASS_CONSUMABLE:
				this->increaseItemCount(existingItem, dest.count);
				break;
			}
		}

		if (!existingItem)
			this->createItem(itemTemplate, dest);
	}
}

void Unit::removeItem(CarriedItem* item)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item->getData()->getItemId());
	NS_ASSERT(tmpl);

	if (tmpl->itemClass == ITEM_CLASS_EQUIPMENT)
	{
		this->applyItemStats(tmpl->itemStats, false);
		m_itemApplicationProcesser->unapplyItem(tmpl);
	}

	item->removeFromWorld();
	item->setOwner(nullptr);
	m_carriedItems[item->getData()->getSlot()] = nullptr;

	delete item;
	item = nullptr;
}

void Unit::removeAllItems()
{
	for (int32 i = 0; i < UNIT_SLOTS_COUNT; ++i)
	{
		CarriedItem* carriedItem = m_carriedItems[i];
		if (!carriedItem)
			continue;

		this->removeItem(carriedItem);
	}
}

CarriedItem* Unit::createItem(ItemTemplate const* itemTemplate, ItemDest const& dest)
{
	CarriedItem* newItem = new CarriedItem();
	newItem->setOwner(this);
	SimpleCarriedItem simple;
	simple.itemId = static_cast<uint16>(itemTemplate->id);
	simple.count = dest.count;
	simple.slot = dest.pos;
	newItem->loadData(simple);

	m_carriedItems[dest.pos] = newItem;
	newItem->addToWorld();

	if (itemTemplate->itemClass == ITEM_CLASS_EQUIPMENT)
	{
		this->applyItemStats(itemTemplate->itemStats, true);
		m_itemApplicationProcesser->applyItem(itemTemplate);
	}

	return newItem;
}

int32 Unit::increaseItemCount(CarriedItem* item, int32 count)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item->getData()->getItemId());
	NS_ASSERT(tmpl);
	NS_ASSERT(tmpl->stackable != ITEM_STACK_NON_STACKABLE);

	int32 newCount = item->getData()->getCount() + count;
	item->getData()->setCount(newCount);

	return newCount;
}

int32 Unit::decreaseItemCount(CarriedItem* item)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item->getData()->getItemId());
	NS_ASSERT(tmpl);
	NS_ASSERT(tmpl->stackable != ITEM_STACK_NON_STACKABLE);

	int32 count = item->getData()->getCount();
	int32 newCount = std::max(0, count - 1);
	item->getData()->setCount(newCount);

	return newCount;
}

uint32 Unit::generateCarriedItemSpawnId()
{
	++m_carriedItemSpawnIdCounter;
	NS_ASSERT(m_carriedItemSpawnIdCounter <= uint32(0xFFFFFF), "CarriedItem spawn id overflow!");
	return m_carriedItemSpawnIdCounter;
}

CarriedItem* Unit::getItem(int32 slot) const
{
	if (slot >= 0 && slot < UNIT_SLOTS_COUNT)
		return m_carriedItems[slot];

	return nullptr;
}

bool Unit::canUseItem(CarriedItem* item) const
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item->getData()->getItemId());
	if (tmpl->itemClass != ITEM_CLASS_CONSUMABLE)
		return false;

	if (!m_itemCooldownProcesser->isReady(item->getData()->getItemId()))
		return false;

	return true;
}

void Unit::takeItem(CarriedItem* item)
{
	int32 newCount = this->decreaseItemCount(item);
	if (newCount <= 0)
		this->removeItem(item);	
}

void Unit::applyItemStats(std::vector<ItemStat> const& stats, bool apply, int32 multiplier)
{
	for (auto const& stat : stats)
	{
		int32 value = stat.value;
		value *= multiplier;
		switch (stat.type)
		{
		case ITEM_STAT_MAX_HEALTH:
			this->handleStatModifier(STAT_MAX_HEALTH, STAT_MODIFIER_VALUE, value, apply);
			break;
		case ITEM_STAT_MAX_HEALTH_PERCENT:
			this->handleStatModifier(STAT_MAX_HEALTH, STAT_MODIFIER_PERCENT, value, apply);
			break;
		case ITEM_STAT_HEALTH_REGEN_RATE:
			this->handleStatModifier(STAT_HEALTH_REGEN_RATE, STAT_MODIFIER_VALUE, value, apply);
			break;
		case ITEM_STAT_HEALTH_REGEN_RATE_PERCENT:
			this->handleStatModifier(STAT_HEALTH_REGEN_RATE, STAT_MODIFIER_PERCENT, value, apply);
			break;
		case ITEM_STAT_ATTACK_RANGE:
			this->handleStatModifier(STAT_ATTACK_RANGE, STAT_MODIFIER_VALUE, value, apply);
			break;
		case ITEM_STAT_ATTACK_RANGE_PERCENT:
			this->handleStatModifier(STAT_ATTACK_RANGE, STAT_MODIFIER_PERCENT, value, apply);
			break;
		case ITEM_STAT_MOVE_SPEED:
			this->handleStatModifier(STAT_MOVE_SPEED, STAT_MODIFIER_VALUE, value, apply);
			break;
		case ITEM_STAT_MOVE_SPEED_PERCENT:
			this->handleStatModifier(STAT_MOVE_SPEED, STAT_MODIFIER_PERCENT, value, apply);
			break;
		case ITEM_STAT_MAX_STAMINA:
			this->handleStatModifier(STAT_MAX_STAMINA, STAT_MODIFIER_VALUE, value, apply);
			break;
		case ITEM_STAT_MAX_STAMINA_PERCENT:
			this->handleStatModifier(STAT_MAX_STAMINA, STAT_MODIFIER_PERCENT, value, apply);
			break;
		case ITEM_STAT_STAMINA_REGEN_RATE:
			this->handleStatModifier(STAT_STAMINA_REGEN_RATE, STAT_MODIFIER_VALUE, value, apply);
			break;
		case ITEM_STAT_STAMINA_REGEN_RATE_PERCENT:
			this->handleStatModifier(STAT_STAMINA_REGEN_RATE, STAT_MODIFIER_PERCENT, value, apply);
			break;
		case ITEM_STAT_DAMAGE:
			this->handleStatModifier(STAT_DAMAGE, STAT_MODIFIER_VALUE, value, apply);
			break;
		case ITEM_STAT_DAMAGE_PERCENT:
			this->handleStatModifier(STAT_DAMAGE, STAT_MODIFIER_PERCENT, value, apply);
			break;
		case ITEM_STAT_DEFENSE:
			this->handleStatModifier(STAT_DEFENSE, STAT_MODIFIER_VALUE, value, apply);
			break;
		default:
			break;
		}
	}
}

void Unit::registerItemEffect(ItemEffect const* effect)
{
	m_ownedItemEffects[effect->type].push_back(effect);
}

void Unit::unregisterItemEffect(ItemEffect const* effect)
{
	auto it = m_ownedItemEffects.find(effect->type);
	if (it != m_ownedItemEffects.end())
	{
		ItemEffectList& effectList = (*it).second;
		effectList.remove(effect);
		if (effectList.empty())
			m_ownedItemEffects.erase(it);
	}
}

bool Unit::hasItemEffectType(ItemEffectType type) const
{
	return m_ownedItemEffects.find(type) != m_ownedItemEffects.end();
}

int32 Unit::getTotalItemEffectModifier(ItemEffectType type) const
{
	int32 modifier = 0;

	auto it = m_ownedItemEffects.find(type);
	if (it != m_ownedItemEffects.end())
	{
		ItemEffectList const& effectList = (*it).second;
		for (ItemEffect const* effect : effectList)
		{
			modifier += effect->value;
		}
	}

	return modifier;
}

bool Unit::canUnlock(ItemBox* itemBox) const
{
	if (!this->isAlive() || !itemBox->isInWorld() || !itemBox->isLocked())
		return false;

	return true;
}

bool Unit::setInUnlock(ItemBox* itemBox)
{
	if (itemBox == m_unlocking)
		return false;

	if (m_unlocking)
		m_unlocking->removeUnlocker(this);
	itemBox->addUnlocker(this);

	m_unlocking = itemBox;

	this->addUnitState(UNIT_STATE_IN_UNLOCK);

	return true;
}

bool Unit::removeUnlockTarget()
{
	if (!m_unlocking)
		return false;

	m_unlocking->removeUnlocker(this);
	m_unlocking = nullptr;

	return true;
}

void Unit::unlockStop()
{
	this->removeUnlockTarget();
	this->clearUnitState(UNIT_STATE_IN_UNLOCK);
}

void Unit::update(NSTime diff)
{
	AttackableObject::update(diff);

	if (!this->isInWorld())
		return;

	this->updateUnsaySmileyTimer(diff);

	if (this->isAlive())
	{
		m_itemApplicationProcesser->update(diff);
		m_itemCooldownProcesser->update(diff);

		this->updateHealthRegenTimer(diff);
		this->updateHealthLossTimer(diff);
		m_rewardManager.update();
		this->updateConcealmentTimer(diff);
		this->updateDangerStateTimer(diff);
		this->updateSlowMoveTimer(diff);
		this->updatePickupTimer(diff);
	}
}

void Unit::addToWorld()
{
	if (this->isInWorld())
		return;

	if(this->getLocator())
		this->getLocator()->addToWorld();

	AttackableObject::addToWorld();
}

void Unit::removeFromWorld()
{
	if (!this->isInWorld())
		return;

	this->combatStop();
	this->pickupStop();
	this->unlockStop();
	this->stopStaminaUpdate();
	if (this->getLocator())
		this->getLocator()->removeFromWorld();

	for (int32 i = 0; i < UNIT_SLOTS_COUNT; ++i)
	{
		CarriedItem* item = m_carriedItems[i];
		if (item)
			item->removeFromWorld();
	}

	this->destroyForNearbyPlayers();

	AttackableObject::removeFromWorld();
}

void Unit::cleanupBeforeDelete()
{
	if (this->isInWorld())
		this->removeFromWorld();

	this->removeAllEnemies();
	m_followerRefManager.clearReferences();
	m_unitHostileRefManager.clearReferences();
	m_awardSourceManager.clearReferences();
	m_itemCollisionRefManager.clearReferences();

	m_itemCooldownProcesser->removeAll();
	m_itemApplicationProcesser->removeAll();
	m_rewardManager.removeAllAwardees();

	for (int32 i = 0; i < UNIT_SLOTS_COUNT; ++i)
	{
		CarriedItem* item = m_carriedItems[i];
		if (item)
		{
			item->setOwner(nullptr);
			delete item;
			m_carriedItems[i] = nullptr;
		}
	}

	AttackableObject::cleanupBeforeDelete();
}

void Unit::sendInitialVisiblePacketsToPlayer(Player* player)
{
	m_itemApplicationProcesser->sendApplicationUpdateAllToPlayer(player, true);
}

bool Unit::hasLineOfSight(Point const& pos) const
{
	TileCoord fromCoord(m_map->getMapData()->getMapSize(), this->getData()->getPosition());
	TileCoord toCoord(m_map->getMapData()->getMapSize(), pos);

	//NS_LOG_DEBUG("behaviors.unit", "Check line of sight from: [%d,%d] to: [%d,%d]", fromCoord.x, fromCoord.x, toCoord.x, toCoord.y);
	bool ret = UnitHelper::checkLineOfSight(m_map, fromCoord, toCoord);

	//NS_LOG_DEBUG("behaviors.unit", "Check Result: %d", ret);

	return ret;
}

void Unit::enterCollision(Projectile* proj, float precision)
{
	if (!proj->getLauncher().isValid() || !this->isAlive() || !this->getMap()->isInBattle())
		return;

	Unit* attacker = proj->getLauncher().getSource();

	// 计算伤害
	int32 damage = attacker->calcDamage(this, proj, precision);
	attacker->dealDamage(this, damage);
}

void Unit::enterCollision(Item* item)
{
	if (!this->canPickup(item))
		return;

	this->pickup(item);
}

void Unit::stayCollision(Item* item)
{
	if (!this->canPickup(item))
		return;

	this->pickup(item);
}

void Unit::leaveCollision(Item* item)
{
	if (item != m_pickingUp)
		return;

	this->pickupStop();
}

int32 Unit::calcDamage(AttackableObject* object, Projectile* projectile, float precision)
{
	ProjectileType projType = projectile->getData()->getProjectileType();
	float damageMultiplier = 1.0f;
	if (projType == PROJECTILE_TYPE_CHARGED)
	{
		int32 attackTakesStamina = this->getData()->getAttackTakesStamina();
		if (projectile->getData()->getChargedStamina() > attackTakesStamina)
		{
			int32 maxStamina = this->getData()->getMaxStamina();
			float tan = ((projectile->getData()->getDamageBonusRatio() * precision + 1.0f) * (float)maxStamina - attackTakesStamina) / (maxStamina - attackTakesStamina);
			float y = tan * (projectile->getData()->getChargedStamina() - attackTakesStamina);
			damageMultiplier = (y + attackTakesStamina) / attackTakesStamina;
		}
	}
	else if (projType == PROJECTILE_TYPE_INTENSIFIED)
		damageMultiplier = 1.0f + projectile->getData()->getDamageBonusRatio();
	int32 damage = (int32)(damageMultiplier * this->getData()->getDamage());

	if(Unit* victim = object->asUnit())
		damage = this->calcDefenseReducedDamage(victim, damage);

	return damage;
}

int32 Unit::calcDefenseReducedDamage(Unit* victim, int32 damage)
{
	float damageMultiplier = 1.0f;
	if (victim->hasItemEffectType(ITEM_EFFECT_DAMAGE_REDUCTION_PERCENT))
	{
		float percent = (float)victim->getTotalItemEffectModifier(ITEM_EFFECT_DAMAGE_REDUCTION_PERCENT) / 100.f;
		damageMultiplier *= (1.0f + percent);
	}
	damageMultiplier *= damage / (float)(damage + victim->getData()->getDefense());

	int32 newDamage = std::max(1, (int32)(damage * damageMultiplier));
	return newDamage;
}

void Unit::dealDamage(Unit* victim, int32 damage)
{
	if (damage <= 0)
		return;

	int32 health = victim->getData()->getHealth();

	int32 realDmg = std::min(damage, health);
	victim->addDamageOfAwardee(this, realDmg);

	if (health > damage)
		victim->receiveDamage(this, damage);
	else
		this->kill(victim);
}

void Unit::dealDamage(ItemBox* itemBox, int32 damage)
{
	if (damage <= 0)
		return;

	int32 health = itemBox->getData()->getHealth();
	if (health > damage)
		itemBox->receiveDamage(this, damage);
	else
		this->kill(itemBox);
}

void Unit::receiveDamage(Unit* attacker, int32 damage)
{
	int32 newHealth = this->getData()->getHealth() - damage;
	this->getData()->setHealth(newHealth);
	this->getData()->addUnitFlag(UNIT_FLAG_DAMAGED);
	if(!this->hasItemEffectType(ITEM_EFFECT_PREVENT_MOVE_SPEED_SLOWED))
		this->slowMoveSpeed(SLOW_MOVE_DURATION);

	this->getUnitHostileRefManager()->updateThreat(UNITTHREAT_ENEMY_HEALTH);
}

void Unit::setFullHealth()
{
	this->getData()->setHealth(this->getData()->getMaxHealth());
	this->resetHealthRegenTimer();
	this->getUnitHostileRefManager()->updateThreat(UNITTHREAT_ENEMY_HEALTH);
}

void Unit::setFullStamina()
{
	this->getData()->setStamina(this->getData()->getMaxStamina());
}

void Unit::normalizeHealth()
{
	int32 maxHealth = this->getData()->getBaseMaxHealth();
	this->getData()->setHealth(maxHealth);
	this->getData()->setMaxHealth(maxHealth);
	this->getData()->clearUnitFlag(UNIT_FLAG_DEATH_LOSE_MONEY);
	this->resetStatModifier(STAT_MAX_HEALTH);
	this->resetHealthRegenTimer();
	this->getUnitHostileRefManager()->updateThreat(UNITTHREAT_ENEMY_HEALTH);
}

void Unit::normalizeAttackRange()
{
	this->getData()->setAttackRange(this->getData()->getBaseAttackRange());
	this->resetStatModifier(STAT_ATTACK_RANGE);
}

void Unit::normalizeStamina()
{
	int32 maxStamina = this->getData()->getBaseMaxStamina();
	this->getData()->setMaxStamina(maxStamina);
	this->getData()->setStamina(maxStamina);
	this->resetStatModifier(STAT_MAX_STAMINA);
}

void Unit::normalizeMoveSpeed()
{
	this->getData()->setMoveSpeed(this->getData()->getBaseMoveSpeed());
	if (this->getLocator())
		this->getLocator()->getData()->setMoveSpeed(this->getData()->getMoveSpeed());
	this->resetStatModifier(STAT_MOVE_SPEED);
}

void Unit::normalizeDamage()
{
	this->getData()->setDamage(this->getData()->getBaseDamage());
	this->resetStatModifier(STAT_DAMAGE);
}

void Unit::normalizeHealthRegenRate()
{
	this->getData()->setHealthRegenRate(this->getData()->getBaseHealthRegenRate());
	this->resetStatModifier(STAT_HEALTH_REGEN_RATE);
}

void Unit::resetAllStatModifiers()
{
	for (uint8 i = 0; i < MAX_STAT_TYPES; ++i)
	{
		this->resetStatModifier((StatType)i);
	}
}

void Unit::resetStatModifier(StatType stat)
{
	m_statModifiers[stat][STAT_MODIFIER_VALUE] = 0;
	m_statModifiers[stat][STAT_MODIFIER_PERCENT] = 0;
}

float Unit::getStatModifierPercent(StatType stat)
{
	int32 amount = m_statModifiers[stat][STAT_MODIFIER_PERCENT];
	return (100.f + (float)amount) / 100.f;
}

int32 Unit::getStatModifierValue(StatType stat)
{
	return m_statModifiers[stat][STAT_MODIFIER_VALUE];
}

void Unit::handleStatModifier(StatType stat, StatModifierType modifierType, int32 amount, bool apply)
{
	m_statModifiers[stat][modifierType] += (apply ? amount : -amount);

	switch (stat)
	{
	case STAT_MAX_HEALTH:
		this->updateMaxHealth();
		break;
	case STAT_HEALTH_REGEN_RATE:
		this->updateHealthRegenRate();
		break;
	case STAT_ATTACK_RANGE:
		this->updateAttackRange();
		break;
	case STAT_MOVE_SPEED:
		this->updateMoveSpeed();
		break;
	case STAT_MAX_STAMINA:
		this->updateMaxStamina();
		break;
	case STAT_STAMINA_REGEN_RATE:
		this->updateStaminaRegenRate();
		break;
	case STAT_DAMAGE:
		this->updateDamage();
		break;
	case STAT_DEFENSE:
		this->updateDefense();
		break;
	default:
		break;
	}
}

void Unit::updateMaxHealth()
{
	float value = (float)this->getData()->getBaseMaxHealth();
	value *= this->getStatModifierPercent(STAT_MAX_HEALTH);
	value += this->getStatModifierValue(STAT_MAX_HEALTH);
	this->getData()->setMaxHealth(int32(value));
}

void Unit::updateHealthRegenRate()
{
	float value = this->getData()->getBaseHealthRegenRate();
	value *= this->getStatModifierPercent(STAT_HEALTH_REGEN_RATE);
	value += this->getStatModifierValue(STAT_HEALTH_REGEN_RATE) / 100.f;
	this->getData()->setHealthRegenRate(value);
}

void Unit::updateAttackRange()
{
	float value = this->getData()->getBaseAttackRange();
	value *= this->getStatModifierPercent(STAT_ATTACK_RANGE);
	value += this->getStatModifierValue(STAT_ATTACK_RANGE);
	value = std::min(value, MAX_ATTACK_RANGE);
	this->getData()->setAttackRange(value);
}

void Unit::updateMoveSpeed()
{
	float value = (float)this->getData()->getBaseMoveSpeed();
	value *= this->getStatModifierPercent(STAT_MOVE_SPEED);
	value += this->getStatModifierValue(STAT_MOVE_SPEED);
	int32 moveSpeed = int32(value);
	moveSpeed = std::min(moveSpeed, MAX_MOVE_SPEED);
	this->getData()->setMoveSpeed(moveSpeed);
	if (this->getLocator())
		this->getLocator()->getData()->setMoveSpeed(moveSpeed);
}

void Unit::updateMaxStamina()
{
	float value = (float)this->getData()->getBaseMaxStamina();
	value *= this->getStatModifierPercent(STAT_MAX_STAMINA);
	value += this->getStatModifierValue(STAT_MAX_STAMINA);
	this->modifyMaxStamina(int32(value));
}

void Unit::updateStaminaRegenRate()
{
	float value = this->getData()->getBaseStaminaRegenRate();
	value *= this->getStatModifierPercent(STAT_STAMINA_REGEN_RATE);
	value += this->getStatModifierValue(STAT_STAMINA_REGEN_RATE);
	this->modifyStaminaRegenRate(value);
}

void Unit::updateDamage()
{
	float value = (float)this->getData()->getBaseDamage();
	value *= this->getStatModifierPercent(STAT_DAMAGE);
	value += this->getStatModifierValue(STAT_DAMAGE);
	this->getData()->setDamage(int32(value));
}

void Unit::updateDefense()
{
	int32 value = this->getStatModifierValue(STAT_DEFENSE);
	this->getData()->setDefense(value);
}

void Unit::stopMoving()
{
	this->restoreMoveSpeed();
}

bool Unit::updatePosition(Point const& newPosition)
{
	bool relocated = (this->getData()->getPosition().x != newPosition.x || this->getData()->getPosition().y != newPosition.y);
	return relocated;
}

void Unit::combatStop()
{
	this->attackStop();
	this->clearUnitState(UNIT_STATE_IN_COMBAT);
}

bool Unit::canCombatWith(Unit* victim) const
{
	if (!this->isAlive() || !victim->isInWorld() || !victim->isAlive())
		return false;

	if (victim == this)
		return false;


	return true;
}

void Unit::addEnemy(Robot* enemy)
{
	auto it = m_enemies.find(enemy);
	if (it == m_enemies.end())
		m_enemies.emplace(enemy);
}

void Unit::removeEnemy(Robot* enemy)
{
	auto it = m_enemies.find(enemy);
	if (it != m_enemies.end())
		m_enemies.erase(it);
}

void Unit::removeAllEnemies()
{
	while (!m_enemies.empty())
	{
		auto it = m_enemies.begin();
		if (!(*it)->removeCombatTarget())
		{
			NS_LOG_ERROR("behaviors.unit", "WORLD: Unit has an enemy but did not combat with it!");
			m_enemies.erase(it);
		}
	}
}

int32 Unit::calcConsumedStamina() const
{
	int32 points = 0;
	int32 stamina = this->getData()->getStamina();
	if (this->hasUnitState(UNIT_STATE_CHARGING))
	{
		int32 chargedStamina = this->getData()->getChargedStamina();
		if (chargedStamina < this->getData()->getAttackTakesStamina())
			points = this->getData()->getAttackTakesStamina() - chargedStamina;
	}
	else
		points = this->getData()->getAttackTakesStamina();
	points = std::min(stamina, points);

	return points;
}

void Unit::relocationTellAttackers()
{
	for (auto it = m_attackers.begin(); it != m_attackers.end(); ++it)
	{
		Unit* attacker = *it;
		if (Robot* robot = attacker->asRobot())
		{
			if (robot->getAttackTarget() && robot->getAttackTarget() != this)
				NS_LOG_DEBUG("behaviors.unit", "Why am I not your attack target?");
		}
		attacker->relocateAttackTarget();
	}
}
