#ifndef __BATTLE_MAP_H__
#define __BATTLE_MAP_H__

#include <bitset>

#include "game/server/protocol/pb/FlashMessage.pb.h"
#include "game/server/protocol/pb/PlayerActionMessage.pb.h"

#include "utilities/Timer.h"
#include "game/grids/GridDefines.h"
#include "game/grids/GridArea.h"
#include "game/grids/Grid.h"
#include "game/world/Locale.h"
#include "game/entities/DataUnit.h"
#include "game/entities/DataItem.h"
#include "game/movement/pathfinding/PrecomputeMap.h"
#include "game/movement/pathfinding/JPSPlus.h"
#include "MapData.h"
#include "SpawnManager.h"
#include "WaypointNode.h"
#include "UnitSpawnPointGenerator.h"

enum BattleUpdateFlag
{
	BATTLE_UPDATEFLAG_NONE					= 0,
	BATTLE_UPDATEFLAG_STATE					= 1 << 0,
	BATTLE_UPDATEFLAG_ALIVE_COUNT			= 1 << 1,
	BATTLE_UPDATEFLAG_MAGICBEAN_COUNT		= 1 << 2,
	BATTLE_UPDATEFLAG_ALL					= BATTLE_UPDATEFLAG_STATE 
											| BATTLE_UPDATEFLAG_ALIVE_COUNT 
											| BATTLE_UPDATEFLAG_MAGICBEAN_COUNT,
};

enum BattleState
{
	BATTLE_STATE_NONE						= 0,
	BATTLE_STATE_PREPARING,
	BATTLE_STATE_IN_PROGRESS,
	BATTLE_STATE_ENDING,
	BATTLE_STATE_ENDED,
};

enum TileFlag
{
	TILE_FLAG_NONE							= 0,
	TILE_FLAG_CLOSED						= 1 << 0,
	TILE_FLAG_ITEM_PLACED					= 1 << 1,
};

class WorldObject;
class Robot;
class Player;
class ObjectUpdater;

class BattleMap
{
	typedef std::unordered_map<std::string /* PlayerID */, GuidUnorderedSet> PlayerIdMappingGuidsMap;

	typedef std::unordered_map<uint32 /* DistrictID */, WaypointNodeMap> DistrictWaypointNodeMap;
	typedef std::multimap<int32 /* Radius */, WaypointNode*> RadialWaypointNodeMultimap;
	typedef std::unordered_map<int32 /* Radius */, int32 /* PointIndex */> RadialPointIndexMap;
	typedef std::vector<std::pair<int32 /* Radius */, TileCoord>> RadialPointList;

public:
	friend class MapClosingTileChecker;

	explicit BattleMap(MapData* data, SpawnManager* spawnManager);
	~BattleMap();

	void onStart();
	void onStop();
	bool isStopped() const { return m_isStopped; }

	void update(NSTime diff);

	void addPlayerToMap(Player* player);
	void removePlayerFromMap(Player* player, bool del);
	template<class T> void addToMap(T* obj);
	template<class T> void removeFromMap(T* obj, bool del);

	void prepareForBattle();
	bool canJoinBattle() const;
	bool isBattleEnded() const { return m_battleState == BATTLE_STATE_ENDED; }
	bool isBattleEnding() const { return m_battleState == BATTLE_STATE_ENDING; }
	bool isInBattle() const { return m_battleState == BATTLE_STATE_IN_PROGRESS;  }
	BattleState getBattleState() const { return m_battleState; }
	NSTime getDurationByBattleState(BattleState state) const;
	NSTime getBattleStateElapsedTime() const { return m_battleStateTimer.getElapsed(); }
	void sendBattleUpdate(Player* player, uint32 updateFlags);
	void sendBattleUpdateToPlayers(uint32 updateFlags);

	void setCombatGrade(CombatGrade const& grade) { m_combatGrade = grade; }
	CombatGrade const& getCombatGrade() const { return m_combatGrade; }
	bool isWithinCombatGrade(uint16 combatPower) const;
	bool isTrainingGround() const { return m_combatGrade.grade == TRAINING_GROUND_COMBAT_GRADE; }
	void resetCombatGrade();

	void setPrimaryLang(LangType lang) { m_primaryLang = lang; }
	LangType getPrimaryLang() const { return m_primaryLang; }

	void setupSafeZone();
	bool isSafeZoneEnabled() const { return m_isSafeZoneEnabled; }
	bool isWithinSafeZone(TileCoord const& tileCoord) const;
	TileCoord const& getSafeZoneCenter() const { return m_safeZoneCenter; }
	int32 getInitialSafeZoneRadius() const { return m_initialSafeZoneRadius; }
	int32 getCurrentSafeZoneRadius() const { return m_currSafeZoneRadius; }
	bool isSafeDistanceMaintained(TileCoord const& tileCoord) const;
	int32 getCurrentSafeDistance() const { return m_currSafeDistance; }
	int32 calcRadiusToSafeZoneCenter(TileCoord const& start);
	bool isDangerAlertTriggered() const { return m_isDangerAlertTriggered; }

	void randomPatrolPoint(TileCoord const& currCoord, TileCoord& result) const;
	bool isWithinPatrolArea(TileCoord const& tileCoord) const;
	bool checkPatrolConditions(TileCoord const& currCoord) const;

	bool isDangerousDistrict(TileCoord const& coord) const;
	WaypointNodeMap const* getDistrictWaypointNodes(TileCoord const& coord) const;
	int32 getExplorableDistrictCount() const { return (int32)m_districtWaypointNodes.size(); }

	int32 getRobotCount() const;
	int32 getItemCount() const;
	int32 getRankTotal() const { return m_rankTotal; }

	int32 getAliveCount() const { return m_aliveCounter; }
	bool isShowdown() const { return m_aliveCounter <= 2; }
	void increaseAliveCounter();
	void decreaseAliveCounter();
	void playerKilled(Player* player);
	void robotKilled(Robot* robot);

	int32 getPlayerInBattleCount() const { return m_playerInBattleCounter; }
	void increasePlayerInBattleCounter();
	void decreasePlayerInBattleCounter();

	std::unordered_map<ObjectGuid, Player*>& getPlayerList() { return m_playerList; }
	MapStoredObjectMapContainer& getObjectsStore() { return m_objectsStore; }
	SpawnManager* getSpawnManager() { return m_spawnManager; }

	void addObjectToRemoveList(WorldObject* obj);
	template<typename T> T* takeReusableObject();
	MapStoredObjectSetContainer& getReusableObjectsStore() { return m_reusableObjectsStore; }

	template<typename VISITOR> void visit(GridCoord const& coord, TypeContainerVisitor<VISITOR, WorldTypeGridContainer>& visitor, bool isCreateGrid = false);
	template<typename VISITOR> void visitGrids(Point const& center, Size const& range, VISITOR& visitor, bool isCreateGrid = false);
	template<typename VISITOR> void visitGrids(Point const& center, float radius, VISITOR& visitor);
	template<typename VISITOR> void visitGridsInMaxVisibleRange(Point const& center, VISITOR& visitor, bool isCreateGrid = false);
	template<typename VISITOR> void visitCreatedGrids(VISITOR& visitor);

	void updateObjectsInGridArea(GridArea const& area, TypeContainerVisitor<ObjectUpdater, WorldTypeGridContainer>& visitor);

	// 单位在寻路时将避开状态为封闭的瓦片
	bool isTileClosed(int32 x, int32 y) const { return this->hasTileFlag(x, y, TILE_FLAG_CLOSED); }
	bool isTileClosed(TileCoord const& coord) const { return this->hasTileFlag(coord, TILE_FLAG_CLOSED); }
	void setTileClosedPosition(Point const& position, bool isClosed = true);
	void setTileClosed(TileCoord const& coord, bool isClosed = true);
	void setTileClosedToNewPosition(Point const& oldPosition, Point const& newPosition);
	void setTileAreaClosed(TileArea const& tileArea, bool isClosed = true);

	void setTileFlags(TileCoord const& coord, uint16 flags);
	void addTileFlag(TileCoord const& coord, uint16 flag);
	bool hasTileFlag(TileCoord const& coord, uint16 flag) const { return this->hasTileFlag(coord.x, coord.y, flag); }
	bool hasTileFlag(int32 x, int32 y, uint16 flag) const;
	void clearTileFlag(TileCoord const& coord, uint16 flag);
	uint16 getTileFlags(TileCoord const& coord) const;

	void findNearestOpenTile(TileCoord const& findCoord, TileCoord& result, bool isExcludeHidingSpots = false) const;
	void findNearestOpenTile(TileCoord const& findCoord, TileCoord const& closestToCoord, TileCoord& result, bool isExcludeHidingSpots = false) const;
	void findNearestOpenTileList(TileCoord const& findCoord, std::vector<TileCoord>& result, bool isExcludeHidingSpots = false) const;

	TileCoord randomOpenTile(bool isExcludeHidingSpots = false) const;
	TileCoord randomOpenTileInArea(TileArea const& area, bool isExcludeHidingSpots = false) const;
	TileCoord randomOpenTileOnCircle(TileCoord const& center, int32 radius, bool isExcludeHidingSpots = false) const;
	TileCoord generateUnitSpawnPoint();

	Size const& getMaxVisibleRange() const { return m_maxVisibleRange; }

	void playerRelocation(Player* player, Point const& newPosition);
	void robotRelocation(Robot* robot, Point const& newPosition);
	void itemRelocation(Item* item, Point const& newPosition);
	void projectileRelocation(Projectile* proj, Point const& newPosition);

	uint16 getMapId() const { return m_mapData->getId(); }
	MapData* getMapData() { return m_mapData; }
	MapData const* getMapData() const { return m_mapData; }

	JPSPlus* getJPSPlus() const { return m_jpsPlus; }

	void addUpdateObject(Object* obj) { m_updateObjects.insert(obj); }
	void removeUpdateObject(Object* obj) { m_updateObjects.erase(obj); }
	void sendObjectUpdates();

	// 向地图上除自己以外的所有玩家发送消息
	void sendGlobalFlashMessage(FlashMessage const& message, Player* self = nullptr);
	void sendGlobalPlayerActionMessage(PlayerActionMessage const& message, Player* self = nullptr);

	uint32 generateRobotSpawnId();
	uint32 generateItemSpawnId();
	uint32 generateItemBoxSpawnId();
	uint32 generateProjectileSpawnId();
	uint32 generateUnitLocatorSpawnId();

private:
	void restart();

	void updateSafeZone();
	void setSafeZoneCenter(TileCoord const& center);

	void updateBattleState(NSTime diff);
	Player* findVictoriousPlayer() const;

	bool isGridMarked(GridCoord const& coord) const { return m_markedGrids.test(coord.y * MAX_NUMBER_OF_GRIDS + coord.x); }
	void markGrid(GridCoord const& coord) { m_markedGrids.set(coord.y * MAX_NUMBER_OF_GRIDS + coord.x, true); }
	void resetAllMarkedGrids() { m_markedGrids.reset(); };

	void initGrids();
	void unloadAllGrids();
	void unloadGrid(GridType* grid);
	void cleanupAllGrids();
	void cleanupGrid(GridType* grid);
	template<typename T> void addToGrid(T* obj, GridCoord const& coord);
	GridType* getGrid(GridCoord const& coord) { return m_grids[coord.x][coord.y]; }
	GridType* getGrid(GridCoord const& coord) const { return m_grids[coord.x][coord.y]; }
	void setGrid(GridCoord const& coord, GridType* grid) { m_grids[coord.x][coord.y] = grid; }
	bool isGridLoaded(GridCoord const& coord) { return this->getGrid(coord) && this->getGrid(coord)->isGridObjectDataLoaded(); }

	void ensureGridLoaded(GridCoord const& coord);
	void ensureGridCreated(GridCoord const& coord);

	void clearClosedTileFlag(TileCoord const& coord);
	void markClosedTileFlag(TileCoord const& coord);

	// 当对象移动到新的Grid时调用
	// GridObject::removeFromGrid()将会影响到ObjectUpdater中的迭代器，故做延迟处理
	void addObjectToNewGrid(WorldObject* target);
	void processObjectToNewGridList();

	void processRelocationNotifies(NSTime diff);

	template<typename T> void deleteFromWorld(T* obj);
	void processObjectRemoveList();
	void cleanupReusableObjects();

	void initWaypointNodes();
	void deleteWaypointNodes();
	void initDistrictWaypointNodes();
	void updateDistrictWaypointNodes();
	void traverseLinkedWaypointNodes(std::unordered_set<WaypointNode*>& visitedNodes, WaypointNode* startNode);
	int32 getMinRadiusWaypointToSafeZoneCenter(TileCoord const& waypoint);

	void updatePatrolPoints();	
	void initJPSPlus();

	void resetMaxVisibleRange();
	void updateMaxVisibleRangeWithPlayer(Player* player);

	bool m_isStopped;

	std::vector<WorldObject*> m_objectToNewGridList;
	std::unordered_set<WorldObject*> m_objectsToRemove;

	std::bitset<MAX_NUMBER_OF_GRIDS * MAX_NUMBER_OF_GRIDS> m_markedGrids;
	GridType* m_grids[MAX_NUMBER_OF_GRIDS][MAX_NUMBER_OF_GRIDS];
	std::list<GridType*> m_gridRefList;
	std::unordered_map<ObjectGuid, Player*> m_playerList;

	MapData* m_mapData;
	PrecomputeMap* m_precomputeMap;
	JPSPlus* m_jpsPlus;
	uint16* m_tileFlagsSet;
	std::map<int32, std::map<int32, TileCoord>> m_splitOpenPoints;
	std::map<int32, std::map<int32, TileCoord>> m_splitUnconcealableOpenPoints;
	UnitSpawnPointGenerator m_unitSpawnPointGenerator;
	Size m_maxVisibleRange;

	std::unordered_set<Object*> m_updateObjects;
	MapStoredObjectMapContainer m_objectsStore;
	MapStoredObjectSetContainer m_reusableObjectsStore;
	SpawnManager* m_spawnManager;

	BattleState m_battleState;
	DelayTimer m_battleStateTimer;
	int32 m_aliveCounter;
	bool m_isAliveCountDirty;
	int32 m_playerInBattleCounter;
	int32 m_rankTotal;

	bool m_isSafeZoneEnabled;
	TileCoord m_safeZoneCenter;
	int32 m_initialSafeZoneRadius;
	int32 m_currSafeZoneRadius;
	int32 m_currSafeDistance;
	bool m_isSafeZoneRelocated;
	bool m_isDangerAlertTriggered;

	CombatGrade m_combatGrade;
	LangType m_primaryLang;

	uint32 m_robotSpawnIdCounter;
	uint32 m_itemBoxSpawnIdCounter;
	uint32 m_itemSpawnIdCounter;
	uint32 m_projectileSpawnIdCounter;
	uint32 m_unitLocatorSpawnIdCounter;

	RadialWaypointNodeMultimap m_radialWaypointNodes;
	DistrictWaypointNodeMap m_districtWaypointNodes;
	WaypointNodeMap m_waypointNodes;
	WaypointNode* m_startWaypointNode;

	bool m_isPatrolPointsDirty;
	RadialPointIndexMap m_patrolPointIndexes;
	RadialPointList m_patrolPointList;
};

template<typename VISITOR>
inline void BattleMap::visit(GridCoord const& coord, TypeContainerVisitor<VISITOR, WorldTypeGridContainer>& visitor, bool isCreateGrid)
{
	if (isCreateGrid)
	{
		ensureGridLoaded(coord);
	}

	if (isGridLoaded(coord))
	{
		GridType* grid = this->getGrid(coord);
		grid->visit(visitor);
	}
}


template<typename VISITOR>
inline void BattleMap::visitGrids(Point const& center, Size const& range, VISITOR& visitor, bool isCreateGrid)
{
	GridArea area = computeGridAreaInRect(center, range, this->getMapData()->getMapSize());
	area.visitGrids(*this, visitor, isCreateGrid);
}

template<typename VISITOR>
inline void BattleMap::visitGrids(Point const& center, float radius, VISITOR& visitor)
{
	GridArea area = computeGridAreaInCircle(center, radius);
	area.visitGrids(*this, visitor);
}

template<typename VISITOR>
inline void BattleMap::visitGridsInMaxVisibleRange(Point const& center, VISITOR& visitor, bool isCreateGrid)
{
	this->visitGrids(center, m_maxVisibleRange, visitor, isCreateGrid);
}

template<typename VISITOR>
void BattleMap::visitCreatedGrids(VISITOR& visitor)
{
	if (m_gridRefList.empty())
		return;

	TypeContainerVisitor<VISITOR, WorldTypeGridContainer> worldObjectVisitor(visitor);

	for (auto it = m_gridRefList.begin(); it != m_gridRefList.end(); ++it)
	{
		GridType* grid = *it;
		grid->visit(worldObjectVisitor);
	}
}

#endif // __BATTLE_MAP_H__