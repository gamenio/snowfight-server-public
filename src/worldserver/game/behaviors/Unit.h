#ifndef __UNIT_H__
#define __UNIT_H__

#include "Common.h"
#include "game/movement/spline/MoveSpline.h"
#include "game/movement/generators/FollowerReference.h"
#include "game/movement/generators/FollowerRefManager.h"
#include "game/entities/DataUnit.h"
#include "game/entities/DataProjectile.h"
#include "game/entities/DataUnitLocator.h"
#include "game/ai/UnitAI.h"
#include "game/combat/UnitHostileRefManager.h"
#include "game/combat/RewardManager.h"
#include "game/combat/AwardSourceManager.h"
#include "game/combat/LaunchRefManager.h"
#include "game/combat/TrajectoryGenerator.h"
#include "game/maps/BattleMap.h"
#include "game/item/ItemApplicationProcesser.h"
#include "game/item/ItemCooldownProcesser.h"
#include "game/collection/ItemCollisionRefManager.h"
#include "WorldObject.h"
#include "CarriedItem.h"
#include "UnitLocator.h"
#include "Item.h"

#define DISCOVER_CONCEALED_UNIT_DISTANCE		80.0f	// Find the distance to concealed unit
#define ITEM_PICKUP_DURATION					1000	// Pickup duration, unit: milliseconds

enum DeathState
{
	DEATH_STATE_ALIVE,
	DEATH_STATE_DEAD,
};

enum UnitState
{
	UNIT_STATE_NONE						= 0,
	UNIT_STATE_MOVING					= 1 << 0,
	UNIT_STATE_IN_COMBAT				= 1 << 1,
	UNIT_STATE_ATTACKING				= 1 << 2,
	UNIT_STATE_IN_COLLECTION			= 1 << 3,
	UNIT_STATE_PICKING_UP				= 1 << 4,
	UNIT_STATE_FORBIDDING_PICKUP		= 1 << 5,
	UNIT_STATE_HIDING					= 1 << 6,
	UNIT_STATE_SEEKING					= 1 << 7,
	UNIT_STATE_CHARGING					= 1 << 8,
	UNIT_STATE_IN_EXPLORATION			= 1 << 9,
	UNIT_STATE_IN_UNLOCK				= 1 << 10,
};

enum StatModifierType
{
	STAT_MODIFIER_VALUE,
	STAT_MODIFIER_PERCENT,
	MAX_STAT_MODIFIER_TYPES,
};

enum BattleOutcome
{
	BATTLE_VICTORY,
	BATTLE_DEFEAT,
};

enum WithdrawalState
{
	WITHDRAWAL_STATE_NONE,
	WITHDRAWAL_STATE_WITHDRAWING,
	WITHDRAWAL_STATE_WITHDREW,
};

enum DangerState
{
	DANGER_STATE_RELEASED,
	DANGER_STATE_ENTERING,
	DANGER_STATE_ENTERED,
};

class Unit : public AttackableObject
{
	typedef std::list<ItemEffect const*> ItemEffectList;
public:
	Unit();
	virtual ~Unit();

	void clearUnitState(uint32 state) { if (hasUnitState(state)) m_unitState &= ~state; }
	bool hasUnitState(uint32 state) const { return (m_unitState & state) != 0; }
	void addUnitState(uint32 state) { if (!hasUnitState(state)) m_unitState |= state; }
	uint32 getUnitState() const { return m_unitState; }

	bool isAlive() const { return m_deathState == DEATH_STATE_ALIVE; }
	virtual void setDeathState(DeathState state);
	virtual void died(Unit* killer) { }
	virtual void kill(Unit* victim) { }
	virtual void kill(ItemBox* itemBox) { }
	void sendDeathMessageToAllPlayers(Unit* killer);

	void fillLoot(Unit* looter, std::vector<LootItem>& itemList);
	void dropLoot(Unit* looter, std::vector<LootItem> const& itemList, int32* spentMoney);

	virtual void withdraw(BattleOutcome outcome, int32 rankNo) {}
	void withdrawDelayed(BattleOutcome outcome, int32 rankNo);
	WithdrawalState getWithdrawalState() const { return m_withdrawalState; }
	BattleOutcome getBattleOutcome() const { return m_battleOutcome; }
	int32 getRankNo() const { return m_rankNo; }

	virtual bool canPickup(Item* supply) const;
	virtual bool pickup(Item* supply);
	Item* getPickupTarget() const { return m_pickingUp; }
	virtual bool pickupStop();
	virtual void receiveItem(Item* supply) {}

	PickupStatus canStoreItem(ItemTemplate const* itemTemplate, int32 count, ItemDest* dest) const;
	virtual void storeItem(ItemTemplate const* itemTemplate, ItemDest const& dest);
	virtual void removeItem(CarriedItem* item);
	void removeAllItems();
	virtual CarriedItem* createItem(ItemTemplate const* itemTemplate, ItemDest const& dest);
	virtual int32 increaseItemCount(CarriedItem* item, int32 count);
	virtual int32 decreaseItemCount(CarriedItem* item);
	uint32 generateCarriedItemSpawnId();
	void resetCarriedItemSpawnIdCounter() { m_carriedItemSpawnIdCounter = 0; }
	CarriedItem* getItem(int32 slot) const;
	virtual bool canUseItem(CarriedItem* item) const;
	virtual void useItem(CarriedItem* item) {}
	virtual void takeItem(CarriedItem* item);

	void applyItemStats(std::vector<ItemStat> const& stats, bool apply, int32 multiplier = 1);
	virtual void registerItemEffect(ItemEffect const* effect);
	virtual void unregisterItemEffect(ItemEffect const* effect);
	bool hasItemEffectType(ItemEffectType type) const;
	int32 getTotalItemEffectModifier(ItemEffectType type) const;

	bool isInUnlock() const { return this->hasUnitState(UNIT_STATE_IN_UNLOCK); }
	virtual bool canUnlock(ItemBox* itemBox) const;
	virtual bool setInUnlock(ItemBox* itemBox);
	ItemBox* getUnlockTarget() const { return m_unlocking; }
	virtual bool removeUnlockTarget();
	virtual void unlockStop();

	UnitAI* getAI() const { return m_ai; }

	virtual void update(NSTime diff) override;
	virtual void addToWorld() override;
	virtual void removeFromWorld() override;

	virtual void cleanupBeforeDelete() override;
	virtual void sendInitialVisiblePacketsToPlayer(Player* player) override;

	virtual bool canSeeOrDetect(WorldObject* object) const { return false; };
	bool hasLineOfSight(Point const& pos) const;
	bool hasLineOfSight(WorldObject* object) const { return hasLineOfSight(object->getData()->getPosition()); }
	// The unit's sight distance
	virtual float getSightDistance() const { return 0; }

	void enterCollision(Projectile* proj, float precision) override;
	void enterCollision(Item* item);
	void stayCollision(Item* item);
	void leaveCollision(Item* item);
	ItemCollisionRefManager* getItemCollisionRefManager() { return &m_itemCollisionRefManager; }

	int32 calcDamage(AttackableObject* object, Projectile* projectile, float precision);
	int32 calcDefenseReducedDamage(Unit* victim, int32 damage);
	virtual void dealDamage(Unit* victim, int32 damage);
	virtual void dealDamage(ItemBox* itemBox, int32 damage);
	virtual void receiveDamage(Unit* attacker, int32 damage);

	void setFullHealth();
	void setFullStamina();
	virtual void modifyMaxStamina(int32 stamina) {}
	virtual void modifyStaminaRegenRate(float regenRate) {}	
	virtual void modifyChargeConsumesStamina(int32 stamina) {}
	void normalizeHealth();
	void normalizeStamina();
	void normalizeAttackRange();
	void normalizeMoveSpeed();
	void normalizeDamage();
	void normalizeHealthRegenRate();

	void resetAllStatModifiers();
	void resetStatModifier(StatType stat);
	void setStatModifier(StatType stat, StatModifierType modifierType, int32 amount) { m_statModifiers[stat][modifierType] = amount; }
	int32 getStatModifier(StatType stat, StatModifierType modifierType) const { return m_statModifiers[stat][modifierType];  }
	float getStatModifierPercent(StatType stat);
	int32 getStatModifierValue(StatType stat);
	void handleStatModifier(StatType stat, StatModifierType modifierType, int32 amount, bool apply);

	void updateMaxHealth();
	void updateHealthRegenRate();
	void updateAttackRange();
	void updateMoveSpeed();
	void updateMaxStamina();
	void updateStaminaRegenRate();
	void updateDamage();
	void updateDefense();

	virtual void stopMoving();
	// Returns true if the position has changed, otherwise returns false
	virtual bool updatePosition(Point const& newPosition);
	virtual void transport(Point const& dest) { }

	bool isInCombat() const { return this->hasUnitState(UNIT_STATE_IN_COMBAT); }
	virtual void combatStop();
	virtual bool canCombatWith(Unit* victim) const;

	void addEnemy(Robot* enemy);
	void removeEnemy(Robot* enemy);
	void removeAllEnemies();

	virtual bool attackStop() { return true; }
	virtual void stopStaminaUpdate() {}
	int32 calcConsumedStamina() const;
	void relocationTellAttackers();
	virtual void relocateAttackTarget() { }
	LaunchRefManager* getLaunchRefManager() { return &m_launchRefManager; }

	FollowerRefManager<Unit>* getFollowerRefManager() { return &m_followerRefManager; }
	UnitHostileRefManager* getUnitHostileRefManager() { return &m_unitHostileRefManager; }

	AwardSourceManager* getAwardSourceManager() { return &m_awardSourceManager; }
	RewardManager* getRewardManager() { return &m_rewardManager; }
	void deleteAwardeeList();
	void addDamageOfAwardee(Unit* attacker, int32 damage);
	float calcReward(Unit* attacker, int32 damagePoints);
	void applyReward(Unit* victim, float reward);
	virtual void giveMoney(int32 money) {}
	virtual void giveXP(int32 xp) { }
	virtual int32 modifyMoney(int32 amount);
	virtual int32 modifyMagicBean(ItemTemplate const* tmpl, int32 amount);
	virtual int32 takeBounty(int32 amount);

	void saySmiley(uint16 code);

	void expose();
	void concealDelayed();
	void concealIfNeeded(Point const& position);
	void updateConcealmentState(TileCoord const& tileCoord);

	void releaseDangerState();
	void enterDangerState();
	virtual void updateDangerState() {}

	virtual UnitLocator* getLocator() const override { return static_cast<UnitLocator*>(m_locator); }
	virtual DataUnit* getData() const override { return static_cast<DataUnit*>(m_data); }

protected:
	void updateHealthRegenTimer(NSTime diff);
	void updateHealthLossTimer(NSTime diff);
	void resetHealthRegenTimer();
	void resetHealthLossTimer();

	float calcProjectileScale(ProjectileType projType);

	void resetWithdrawalTimer();
	void updateWithdrawalTimer(NSTime diff);

	void updateUnsaySmileyTimer(NSTime diff);
	void resetUnsaySmileyTimer();
	void unsaySmiley();

	void updateConcealmentTimer(NSTime diff);
	void resetConcealmentTimer();
	virtual void concealed();

	void updateDangerStateTimer(NSTime diff);
	void resetDangerStateTimer();

	void slowMoveSpeed(NSTime duration);
	void updateSlowMoveTimer(NSTime diff);
	void restoreMoveSpeed();

	void updatePickupTimer(NSTime diff);

	PickupStatus canStoreItemInInventoryCustomSlots(ItemTemplate const* itemTemplate, int32 count, ItemDest* dest) const;
	PickupStatus canStoreStackableItemInSpecificSlot(int32 slot, ItemTemplate const* itemTemplate, int32 count, ItemDest* dest) const;
	PickupStatus canStoreItemInEquipmentSlot(ItemTemplate const* itemTemplate, ItemDest* dest) const;
	int32 getCannotStoreItemCount(ItemTemplate const* itemTemplate, int32 itemPos, int32 count) const;

	int32 addExperience(int32 xp);
	virtual void createDroppedItem(ItemTemplate const* tmpl, TileCoord const& spawnPoint, int32 count);

	uint32 m_unitState;
	DeathState m_deathState;
	DangerState m_dangerState;
	int32 m_statModifiers[MAX_STAT_TYPES][MAX_STAT_MODIFIER_TYPES];
	std::unordered_set<Robot*> m_enemies;

	int32 m_healthRegenInterval;
	DelayTimer m_healthRegenTimer;
	DelayTimer m_healthLossTimer;

	FollowerRefManager<Unit> m_followerRefManager;
	UnitHostileRefManager m_unitHostileRefManager;
	LaunchRefManager m_launchRefManager;
	ItemCollisionRefManager m_itemCollisionRefManager;

	AwardSourceManager m_awardSourceManager;
	RewardManager m_rewardManager;

	DelayTimer m_unsaySmileyTimer;
	DelayTimer m_concealmentTimer;

	BattleOutcome m_battleOutcome;
	int32 m_rankNo;
	WithdrawalState m_withdrawalState;
	DelayTimer m_withdrawalTimer;
	DelayTimer m_dangerStateTimer;

	ItemBox* m_unlocking;

	Item* m_pickingUp;
	DelayTimer m_pickupTimer;
	ItemApplicationProcesser* m_itemApplicationProcesser;
	ItemCooldownProcesser* m_itemCooldownProcesser;
	CarriedItem* m_carriedItems[UNIT_SLOTS_COUNT];
	uint32 m_carriedItemSpawnIdCounter;
	std::unordered_map<uint32/* ItemEffectType */, ItemEffectList> m_ownedItemEffects;

	UnitAI* m_ai;

	DelayTimer m_slowMoveTimer;
	bool m_isSlowMoveEnabled;
};

#endif // __UNIT_H__

