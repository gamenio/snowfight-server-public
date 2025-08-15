#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "game/server/protocol/pb/AttackInfo.pb.h"
#include "game/server/protocol/pb/ItemActionMessage.pb.h"

#include "utilities/TimeUtil.h"
#include "game/entities/DataPlayer.h"
#include "game/ai/PlayerAI.h"
#include "game/server/WorldSession.h"
#include "Unit.h"

#define MOVING_HEARTBEAT_INTERVAL_MAX				0.2f	// Maximum interval for client moving heartbeat, unit: seconds
#define STAMINA_SYNC_INTERVAL						0.2f	// Client stamina synchronization interval, unit: seconds
// The maximum step length allowed for player moving, unit: points
static const float MAX_STEP_LENGTH = MOVING_HEARTBEAT_INTERVAL_MAX * MAX_MOVE_SPEED + 1.0f;


enum AttackFlag
{
	ATTACK_FLAG_NONE			= 0,
	ATTACK_FLAG_ALL_OUT			= 1 << 0,
};

class Player : public Unit, public GridObject<Player>
{
	typedef std::list<std::pair<uint32 /* ItemId */, int32 /* Count */>> ItemHistoryList;

public:
	explicit Player(WorldSession* session);
	virtual ~Player();

	void update(NSTime diff) override;

	void addToWorld() override;
	void removeFromWorld() override;

	void sendInitSelf();
	void sendInitialPacketsAfterAddToMap();
	void sendResetPackets();

	void spawn(BattleMap* map);
	void died(Unit* killer) override;
	void resetStats();
	void kill(Unit* victim) override;
	void kill(ItemBox* itemBox) override;
	void setLevelAndXP(uint8 level, int32 xp);
	void withdraw(BattleOutcome outcome, int32 rankNo) override;

	void giveXP(int32 xp) override;
	void giveMoney(int32 money) override;
	int32 modifyMagicBean(ItemTemplate const* tmpl, int32 amount) override;

	bool pickup(Item* supply) override;
	bool pickupStop() override;
	bool canPickup(Item* supply) const override;
	void receiveItem(Item* supply) override;

	void removeItem(CarriedItem* item) override;
	CarriedItem* createItem(ItemTemplate const* itemTemplate, ItemDest const& dest) override;
	int32 increaseItemCount(CarriedItem* item, int32 count) override;
	int32 decreaseItemCount(CarriedItem* item) override;
	void useItem(CarriedItem* item) override;
	void takeItem(CarriedItem* item) override;

	void setSession(WorldSession* session);
	WorldSession* getSession() const { return m_session; }
	void discardSession();

	bool loadData(uint32 templateId);
	virtual DataPlayer* getData() const override { return static_cast<DataPlayer*>(m_data); }

	virtual void setMap(BattleMap* map);
	PlayerAI* getAI() const { return static_cast<PlayerAI*>(m_ai); }

	void modifyMaxStamina(int32 stamina) override;
	void modifyStaminaRegenRate(float regenRate) override;
	void modifyChargeConsumesStamina(int32 stamina) override;

	bool canSeeOrDetect(WorldObject* object) const override;
	// The player's sight distance is determined by the size of the terminal device screen
	virtual float getSightDistance() const override { return 0; }
	// Check if the object is within the visible range
	bool isWithinVisibleRange(WorldObject* object) const;

	bool canTrack(WorldObject* object) const;
	bool hasTrackingAtClient(WorldObject* object) const;
	void removeTrackingFromClient(WorldObject* object);

	// Update the traceability of the specified target object
	void updateTraceabilityOf(WorldObject* target);
	void updateTraceabilityOf(WorldObject* target, UpdateObject& update);
	// Update the traceability of units near the player
	void updateTraceabilityForPlayer();
	// Update the traceability of objects in the player client list
	void updateTraceabilityInClient();

	virtual bool updatePosition(Point const& newPosition) override;
	void stopMoving() override;
	void transport(Point const& dest) override;

	void attack(AttackInfo const& attackInfo);
	bool attackStop() override;
	bool isContinuousAttack() const { return m_isContinuousAttack; }
	void stopContinuousAttack();
	void charge();
	void chargeStop();
	void stopStaminaUpdate() override;
	uint32 getAttackTotal() const { return (uint32)m_normalAttackCount + m_continuousAttackCount + m_chargedAttackCount; }

	void receiveDamage(Unit* attacker, int32 damage) override;
	void dealDamage(Unit* victim, int32 damage) override;
	void dealDamage(ItemBox* itemBox, int32 damage) override;
	bool canCombatWith(Unit* victim) const override;

	// Update the visibility of units near the player
	void updateVisibilityForPlayer();
	// Update the visibility of objects in the player client list
	void updateVisibilityInClient();
	// Update the visibility of the specified target object
	void updateVisibilityOf(WorldObject* target);
	void updateVisibilityOf(WorldObject* target, UpdateObject& update, std::unordered_set<WorldObject*>& visibleNow);
	// Update player visibility
	void updateObjectVisibility(bool forced = false) override;

	void updateDangerState() override;

	bool hasAtClient(WorldObject* object) const;
	void removeFromClient(WorldObject* object);
	void cleanupAfterUpdate() override;

	void sendCreateClientObjectsForPlayer();
	void sendCreateClientLocatorsForPlayer();
	void sendBattleResult(BattleOutcome outcome, int32 rankNo);
	void sendLaunchResult(uint32 attackInfoCounter, int32 status);

	void updateItemPickupResultForPlayer();
	void sendItemPickupResult(PickupStatus status);
	void sendItemCooldownListForPlayer();
	void sendItemUseResult(ObjectGuid const& guid, ItemUseStatus status);
	void sendItemActionMessage(ItemActionMessage::ActionType actionType, ItemTemplate const* itemTemplate);
	void sendItemReceivedMessage(ItemTemplate const* itemTemplate, int32 count);

	void sendRewardXPMessage(int32 xp);
	void sendRewardMoneyMessage(int32 money);

	std::string getStatusInfo();

private:
	void concealed() override;

	void attack(float direction, uint32 attackInfoCounter);
	void updateAttackTimer(NSTime diff);
	void resetAttackTimer();

	void updateUnlockerTimer(NSTime diff);
	void resetUnlockerTimer();

	void calcRewardForRanking(int32 rankNo, int32 rankTotal, int32* giveMoney, int32* giveXP);

	WorldSession* m_session;
	GuidUnorderedSet m_clientGuids;
	GuidUnorderedSet m_clientTrackingGuids;

	uint32 m_templateId;

	int32 m_numAttacks;
	bool m_isContinuousAttack;
	float m_attackDirection;
	IntervalTimer m_attackTimer;

	DelayTimer m_unlockerTimer;

	uint16 m_hitCount;
	uint16 m_projectionCount;
	uint16 m_normalAttackCount;
	uint16 m_chargedAttackCount;
	uint16 m_continuousAttackCount;
	int32 m_earnedXP;
	uint16 m_itemBoxCount;
	int32 m_magicBeanCount;
	ItemHistoryList m_itemHistoryList;
};


#endif // __PLAYER_H__
