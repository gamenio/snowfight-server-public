#ifndef __ROBOT_H__
#define __ROBOT_H__

#include "game/server/protocol/Opcode.h"
#include "game/entities/DataRobot.h"
#include "game/maps/ExplorArea.h"
#include "game/ai/RobotAI.h"
#include "game/movement/spline/RobotMoveSpline.h"
#include "game/movement/MotionMaster.h"
#include "game/combat/UnitThreatManager.h"
#include "game/combat/ProjectileThreatManager.h"
#include "game/combat/RobotStaminaUpdater.h"
#include "game/collection/WishManager.h"
#include "game/tiles/TileArea.h"
#include "Unit.h"


#define	SIGHT_DISTANCE							240		// 机器人默认的视野范围
extern const int32 SIGHT_DISTANCE_IN_TILES;				// 以瓦片为单位的视野距离
#define ABANDON_ATTACK_DISTANCE					380		// 机器人放弃追击目标的距离（不小于视野范围）

struct SimpleRobot
{
	SimpleRobot() :
		templateId(0),
		spawnPoint(TileCoord::INVALID),
		level(0),
		stage(0),
		reactFlags(0),
		proficiencyLevel(0),
		natureType(NATURE_CAREFUL)
	{}

	uint32 templateId;
	std::string name;
	std::string country;
	TileCoord spawnPoint;
	uint8 level;
	uint8 stage;
	uint32 reactFlags;
	uint8 proficiencyLevel;
	uint32 natureType;
};

enum CombatState
{
	COMBAT_STATE_CHASE,
	COMBAT_STATE_ESCAPE,
	MAX_COMBAT_STATE,
};

enum UnlockState
{
	UNLOCK_STATE_CHASE,
	UNLOCK_STATE_GO_BACK,
};

enum CollectionState
{
	COLLECTION_STATE_COLLECT,
	COLLECTION_STATE_COMBAT,
};

class Robot :public Unit, public GridObject<Robot>
{
	typedef std::unordered_map<int32 /* AreaIndex */, uint32 /* Order */> ExplorAreaOrderMap;
	typedef std::unordered_map<uint32 /* DistrictID */, std::list<ExplorArea>> DistrictExplorAreaListMap;
	typedef std::unordered_map<int32 /* AreaIndex */, std::list<ExplorArea>::iterator> ExplorAreaListIteratorMap;
	typedef std::unordered_map<uint32 /* DistrictID */, uint32 /* Counter */> DistrictCounterMap;
	typedef std::list<std::pair<ExplorArea, NSTime>> TimedExplorAreaList;
	typedef std::unordered_map<int32 /* AreaIndex */, TimedExplorAreaList::iterator> TimedExplorAreaListIteratorMap;

	typedef std::list<std::pair<TileCoord, NSTime>> TimedTileList;
	typedef std::unordered_map<int32 /* TileIndex */, TimedTileList::iterator> TimedTileListIteratorMap;

	enum AttackSpeed
	{
		ATTACK_SPEED_NONE,
		ATTACK_SPEED_FAST,
		ATTACK_SPEED_SLOW
	};

public:
	Robot();

	virtual ~Robot();

	void addToWorld() override;
	void removeFromWorld() override;
	void update(NSTime diff) override;

	void cleanupBeforeDelete() override;

	void setDeathState(DeathState state) override;
	void died(Unit* killer) override;
	void kill(Unit* victim) override;
	void kill(ItemBox* itemBox) override;
	void giveMoney(int32 money) override;
	void giveXP(int32 xp) override;
	void withdraw(BattleOutcome outcome, int32 rankNo) override;

	bool canHide(TileCoord const& hidingSpot) const;
	TileCoord const& getHidingSpot() const { return m_hidingSpot; }
	bool setHidingSpot(TileCoord const& tileCoord);
	void hideStop();

	bool canSeek(TileCoord const& hidingSpot) const;
	bool isHidingSpotChecked(TileCoord const& tileCoord) const;
	void markHidingSpotChecked(TileCoord const& tileCoord);
	void clearCheckedHidingSpots();

	bool isInCollection() const { return this->hasUnitState(UNIT_STATE_IN_COLLECTION); }
	bool canCollect(Item* supply) const;
	bool setCollectTarget(Item* supply);
	bool removeCollectTarget();
	Item* getSupply() const { return m_collecting; }
	void collectStop();

	bool checkCollectionCollectConditions() const;
	bool checkCollectionCombatConditions() const;
	bool updateCollectionState();
	CollectionState getCollectionState() const { return m_collectionState; }
	void setCollectionState(CollectionState state) { m_collectionState = state; }

	void addWish(Item* supply);
	Item* selectWish();
	void deleteWishList();
	bool isInterestedTarget(Item* item) const;
	WishManager* getWishManager() { return &m_wishManager; }

	bool canPickup(Item* supply) const override;
	bool pickup(Item* supply) override;
	bool pickupStop() override;
	void receiveItem(Item* supply) override;

	void storeItem(ItemTemplate const* itemTemplate, ItemDest const& dest);
	void removeItem(CarriedItem* item) override;
	CarriedItem* createItem(ItemTemplate const* itemTemplate, ItemDest const& dest) override;
	bool canUseItem(CarriedItem* item) const override;
	void useItem(CarriedItem* item) override;
	void unregisterItemEffect(ItemEffect const* effect);

	bool canUnlock(ItemBox* itemBox) const override;
	void unlockStop() override;

	bool checkUnlockChaseConditions() const;
	bool checkUnlockGoBackConditions() const;
	bool updateUnlockState();
	UnlockState getUnlockState() const { return m_unlockState; }
	void setUnlockState(UnlockState state) { m_unlockState = state; }

	bool isInExploration() const { return this->hasUnitState(UNIT_STATE_IN_EXPLORATION); }
	bool canExploreArea() const;
	void exploreStop();

	RobotMoveSpline* getMoveSpline() const { return m_moveSpline; }
	MotionMaster* getMotionMaster() const { return m_motionMaster; }
	void stopMoving() override;
	void setAI(RobotAIType type);
	RobotAI* getAI() const { return static_cast<RobotAI*>(m_ai); }

	bool canCombatWith(Unit* victim) const override;
	bool setInCombatWith(Unit* victim);
	bool removeCombatTarget();
	void combatStop() override;
	Unit* getVictim() const { return m_combating; }
	float calcOptimalDistanceToDodgeTarget(Unit* target) const;
	float calcOptimalDistanceToDodgeTarget(ItemBox* target) const;

	bool checkCombatChaseConditions() const;
	bool checkCombatEscapeConditions() const;
	bool updateCombatState();
	CombatState getCombatState() const { return m_combatState; }
	void setCombatState(CombatState state) { m_combatState = state; }
	bool canActiveAttack(Unit* victim) const;

	void receiveDamage(Unit* attacker, int32 damage) override;
	void modifyMaxStamina(int32 stamina) override;
	void modifyStaminaRegenRate(float regenRate) override;
	void modifyChargeConsumesStamina(int32 stamina) override;

	bool canSeeOrDetect(WorldObject* object) const override;
	virtual float getSightDistance() const override { return SIGHT_DISTANCE; }
	bool isWithinEffectiveRange(AttackableObject* target) const;
	bool lockingTarget(AttackableObject* target) const;
	float calcEffectiveRange(AttackableObject* target) const;

	void attack();
	bool attackStop() override;
	bool setAttackTarget(AttackableObject* target);
	AttackableObject* getAttackTarget() const { return m_attacking; }
	void relocateAttackTarget() override;
	void stopStaminaUpdate() override;

	bool isAttackReady() const;
	void setAttackTimer(NSTime interval, NSTime delay = 0);
	void resetAttackTimer();

	void charge();
	void chargeStop();
	bool canCharge() const;

	Unit* selectVictim();
	float calcThreat(Unit* victim, UnitThreatType type, float variant = 0.f);
	float applyThreatModifier(UnitThreatType type, float threat);
	UnitThreatManager* getUnitThreatManager() { return &m_unitThreatManager; }
	void deleteUnitThreatList();
	void addThreat(Unit* victim, UnitThreatType type, float variant = 0.f);
	bool isWithinCriticalThreatRange(Unit* victim) const;
	bool isHostileTarget(Unit* victim) const;
	bool isPotentialThreat(Unit* victim) const;

	Projectile* selectProjectileToDodge();
	ProjectileThreatManager* getProjectileThreatManager() { return &m_projThreatManager; }
	void deleteProjectileThreatList();
	void addThreat(Projectile* proj);
	bool isHostileTarget(Projectile* proj) const;
	bool isPotentialThreat(Projectile* proj) const;

	bool loadData(SimpleRobot const& simpleRobot, BattleMap* map);
	bool reloadData(SimpleRobot const& simpleRobot, BattleMap* map);
	virtual DataRobot* getData() const override { return static_cast<DataRobot*>(m_data); }

	bool updatePosition(Point const& newPosition) override;
	void transport(Point const& dest) override;
	void setFacingToObject(WorldObject* object);

	void cleanupAfterUpdate() override;
	// 发送动作信息给周围能够看到当前机器人的玩家
	void sendMoveOpcodeToNearbyPlayers(Opcode opcode, MovementInfo& movement);
	void sendMoveSync();
	void sendStaminaOpcodeToNearbyPlayers(Opcode opcode, StaminaInfo& stamina);
	// 发送位置信息给周围能够看到当前定位器的玩家
	void sendLocationInfoToNearbyPlayers(LocationInfo& location);

	void updateDangerState() override;

	void increaseStepCount();
	void resetStepCounter() { m_stepCount = 0; }

	int32 getExplorAreaSize() const { return m_explorAreaSize; }
	int32 getExplorWidth() const { return m_explorWidth; }
	int32 getExplorHeight() const { return m_explorHeight; }
	Point getExplorAreaOffset() const { return m_explorAreaOffset; }
	ExplorArea getExplorArea(Point const& pos) const;
	bool isExplorAreaContainsPoint(ExplorArea const& area, Point const& point) const;
	Point computeExplorAreaPosition(ExplorArea const& area) const;

	void increaseDistrictCounter(TileCoord const& coordInDistrict);
	uint32 getDistrictCount(TileCoord const& coordInDistrict) const;
	bool isAllDistrictsExplored() const;
	void removeDistrictCounter(uint32 districtId);

	bool selectWaypointExplorArea(ExplorArea const& currArea, ExplorArea& waypointArea, TileCoord& waypoint);
	void getSafePointFromWaypointExtent(TileCoord const& waypoint, TileCoord& result);
	void setSourceWaypoint(TileCoord const& waypoint);
	TileCoord const& getSourceWaypoint() const { return m_sourceWaypoint; }
	ExplorArea const& getCurrentWaypointExplorArea() const { return  m_currWaypointExplorArea; }
	void setCurrentWaypointExplorArea(ExplorArea const& area) { m_currWaypointExplorArea = area; }

	ExplorArea const& getGoalExplorArea() const { return  m_goalExplorArea; }
	void setGoalExplorArea(ExplorArea const& area) { m_goalExplorArea = area; }
	void setInPatrol(bool isInPatrol) { m_isInPatrol = isInPatrol; }
	bool isInPatrol() const { return m_isInPatrol; }

	void resetExploredAreas();
	void markAreaExplored(ExplorArea const& area);
	uint32 getExplorOrder(ExplorArea const& area);

	void increaseWorldExplorCounter();
	int32 getWorldExplorCount() const { return m_worldExplorCount; }
	bool hasWorldExplored() const;

	void addUnexploredExplorArea(ExplorArea const& area);
	void removeUnexploredExplorArea(ExplorArea const& area);
	void removeAllUnexploredExplorAreas();
	void updateUnexploredExplorAreas(ExplorArea const& currArea);
	bool selectNextUnexploredExplorArea(ExplorArea const& currArea, ExplorArea& result);
	bool hasUnexploredExplorArea(ExplorArea const& currArea);

	void addExcludedExplorArea(ExplorArea const& area);
	bool isExplorAreaExcluded(ExplorArea const& area) const;
	void updateExcludedExplorAreas();

private:
	void initializeAI(RobotAIType type);

	void attackDelayed();
	void attackInternal();

	void updateAttackTimer(NSTime diff);
	void resetAttackLostTime() { m_attackLostTime = 0; }
	void updateAttackMode();
	void startNormalAttack(AttackSpeed speed);
	void stopNormalAttack();
	bool startContinuousAttack();
	void stopContinuousAttack();

	float calcAimingDirection(AttackableObject* target) const;

	void concealed() override;
	void relocateHidingSpot(TileCoord const& currCoord);

	void updateAttackDelayTimer(NSTime diff);
	void stopAttackDelayTimer();
	void startHandDownTimer();
	void stopHandDownTimer();
	void updateHandDownTimer(NSTime diff);

	void setupExplorAreas(MapData* mapData, Point const& centerInArea);
	void cleanupExplorAreas();
	bool findBestWaypoint(TileCoord const& findCoord, TileCoord& result);

	bool checkUseItemChaseConditions(CarriedItem* item) const;
	bool checkUseItemFindEnemiesConditions(CarriedItem* item) const;
	bool checkUseItemFirstAidConditions(CarriedItem* item) const;
	bool checkUseItemEscapeConditions(CarriedItem* item) const;

	void createDroppedItem(ItemTemplate const* tmpl, TileCoord const& spawnPoint, int32 count) override;

	void updateFavorableEquipmentCount();
	void updateFavorableMagicBeanDiff();

	bool canUnlockInCombat() const;
	bool canCollectInCombat() const;

	uint32 m_templateId;

	RobotMoveSpline* m_moveSpline;
	MotionMaster* m_motionMaster;
	RobotStaminaUpdater* m_staminaUpdater;

	TileCoord m_hidingSpot;
	TimedTileList m_checkedHidingSpotList;
	TimedTileListIteratorMap m_checkedHidingSpotLookupTable;

	Unit* m_combating;
	CombatState m_combatState;
	int32 m_favorableEquipmentCount;
	int32 m_favorableMagicBeanDiff;

	AttackableObject* m_attacking;
	bool m_isAttackModeDirty;
	NSTime m_attackTimer;
	NSTime m_attackLostTime;
	NSTime m_attackInterval;
	NSTime m_attackMinInterval;
	NSTime m_attackMaxInterval;
	AttackSpeed m_attackSpeed;
	int32 m_numAttacks;
	bool m_isContinuousAttack;
	float m_attackDirection;

	DelayTimer m_attackDelayTimer;
	bool m_isAttackDelayed;
	bool m_isHandDownTimerEnabled;
	DelayTimer m_handDownTimer;

	Item* m_collecting;
	CollectionState m_collectionState;
	WishManager m_wishManager;
	int32 m_equipmentCount;

	UnlockState m_unlockState;

	UnitThreatManager m_unitThreatManager;
	float m_unitThreatModifiers[MAX_COMBAT_STATE][MAX_UNITTHREAT_TYPES];
	ProjectileThreatManager m_projThreatManager;

	int32 m_stepCount;
	int32 m_worldExplorCount;

	int32 m_explorAreaSize;
	int32 m_explorWidth;
	int32 m_explorHeight;
	Point m_explorAreaOffset;

	uint32 m_explorOrderCounter;
	ExplorAreaOrderMap m_explorAreaFlags;
	DistrictExplorAreaListMap m_districtUnexploredExplorAreas;
	ExplorAreaListIteratorMap m_unexploredExplorAreaLookupTable;
	TileCoord m_sourceWaypoint;
	DistrictCounterMap m_districtCounters;
	ExplorArea m_goalExplorArea;
	ExplorArea m_currWaypointExplorArea;
	bool m_isInPatrol;

	TimedExplorAreaList m_excludedExplorAreaList;
	TimedExplorAreaListIteratorMap m_excludedExplorAreaLookupTable;
};

#endif // __ROBOT_H__
