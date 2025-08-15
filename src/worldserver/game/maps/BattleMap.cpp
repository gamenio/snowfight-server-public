#include "BattleMap.h"

#include "game/server/protocol/pb/BattleUpdate.pb.h"

#include "logging/Log.h"
#include "game/dynamic/TypeContainerVisitor.h"
#include "game/grids/GridNotifiers.h"
#include "game/grids/ObjectGridLoader.h"
#include "game/behaviors/Robot.h"
#include "game/entities/updates/UpdateObject.h"
#include "game/world/ObjectMgr.h"
#include "game/world/World.h"

#define USE_MAP_CENTER_AS_SAFEZONE_CENTER		0		// Use the center of the map as the center of the safe zone

class ReusableObjectsStoreCleaner
{
public:
	ReusableObjectsStoreCleaner() {}
	~ReusableObjectsStoreCleaner() {}

	template<typename T>
	void visit(std::unordered_set<T*>& objects)
	{
		while (!objects.empty())
		{
			auto it = objects.begin();
			T* obj = *it;
			objects.erase(it);
			delete obj;
		}
	}
};

BattleMap::BattleMap(MapData* data, SpawnManager* spawnManager) :
	m_isStopped(true),
	m_mapData(data),
	m_precomputeMap(nullptr),
	m_jpsPlus(nullptr),
	m_tileFlagsSet(nullptr),
	m_unitSpawnPointGenerator(this),
	m_maxVisibleRange(Size::ZERO),
	m_spawnManager(spawnManager),
	m_battleState(BATTLE_STATE_NONE),
	m_aliveCounter(0),
	m_isAliveCountDirty(false),
	m_playerInBattleCounter(0),
	m_rankTotal(0),
	m_isSafeZoneEnabled(false),
	m_initialSafeZoneRadius(0),
	m_currSafeZoneRadius(0),
	m_currSafeDistance(0),
	m_isSafeZoneRelocated(false),
	m_isDangerAlertTriggered(false),
	m_combatGrade(),
	m_primaryLang(LANG_enUS),
	m_robotSpawnIdCounter(0),
	m_itemBoxSpawnIdCounter(0),
	m_itemSpawnIdCounter(0),
	m_projectileSpawnIdCounter(0),
	m_unitLocatorSpawnIdCounter(0),
	m_startWaypointNode(nullptr),
	m_isPatrolPointsDirty(false)
{
	m_spawnManager->setMap(this);
	m_unitSpawnPointGenerator.initialize();

	int32 nTiles = (int32)(m_mapData->getMapSize().width * m_mapData->getMapSize().height);
	m_tileFlagsSet = new uint16[nTiles]();
	this->initGrids();
	this->initWaypointNodes();
	this->initJPSPlus();
}

BattleMap::~BattleMap()
{
	this->cleanupReusableObjects();
	this->unloadAllGrids();
	this->deleteWaypointNodes();

	if (m_jpsPlus)
	{
		delete m_jpsPlus;
		m_jpsPlus = nullptr;
	}
	if (m_precomputeMap)
	{
		delete m_precomputeMap;
		m_precomputeMap = nullptr;
	}

	if (m_tileFlagsSet)
	{
		delete[] m_tileFlagsSet;
		m_tileFlagsSet = nullptr;
	}
	m_spawnManager->setMap(nullptr);
	m_spawnManager = nullptr;

	// Before deleting the map, you must first call the unloadAll() function
}

void BattleMap::onStart()
{
	if (!m_isStopped)
		return;

	this->restart();
	m_isStopped = false;
}

void BattleMap::onStop()
{
	if (m_isStopped)
		return;

	this->cleanupAllGrids();

	m_battleStateTimer.reset();
	m_battleState = BATTLE_STATE_NONE;
	m_isSafeZoneRelocated = false;
	m_isDangerAlertTriggered = false;

	NS_ASSERT(m_aliveCounter <= 0);
	NS_ASSERT(m_playerInBattleCounter <= 0);

	m_isStopped = true;
}

void BattleMap::update(NSTime diff)
{
	if (this->isStopped() || this->isBattleEnded())
		return;

	this->updatePatrolPoints();
	this->resetAllMarkedGrids();

	// Update players
	this->resetMaxVisibleRange();
	ObjectUpdater updater(diff);
	TypeContainerVisitor<ObjectUpdater, WorldTypeGridContainer> updaterVisitor(updater);
	for (auto it = m_playerList.begin(); it != m_playerList.end(); ++it)
	{
		Player* player = (*it).second;

		this->updateMaxVisibleRangeWithPlayer(player);
		player->update(diff);

		// Update passive objects in the nearby active state grid, such as items
		GridArea area = computeGridAreaInRect(player->getData()->getPosition(), player->getData()->getVisibleRange(), this->getMapData()->getMapSize());
		this->updateObjectsInGridArea(area, updaterVisitor);
	}

	// Update active objects other than players
	ObjectUpdateNotifier updateNotifier(*this, diff);
	this->visitCreatedGrids(updateNotifier);

	this->processObjectToNewGridList();
	this->processRelocationNotifies(diff);
	this->processObjectRemoveList();

	// Update object spawning manager
	m_spawnManager->update(diff);

	this->sendObjectUpdates();

	this->updateBattleState(diff);
}

template<typename T>
void BattleMap::deleteFromWorld(T* obj)
{
	delete obj;
}

template<>
void BattleMap::deleteFromWorld(Player* player)
{
	removeUpdateObject(player);
	delete player;
}

void BattleMap::addPlayerToMap(Player* player)
{
	// Calculate the player's grid coordinates
	GridCoord coord = computeGridCoordForPosition(player->getData()->getPosition());
	this->ensureGridLoaded(coord);
	this->addToGrid(player, coord);

	this->updateMaxVisibleRangeWithPlayer(player);

	player->setMap(this);
	player->addToWorld();

	player->sendInitSelf();

	player->updateObjectVisibility();
	player->updateObjectTraceability();
	player->updateObjectSafety();
}

void BattleMap::removePlayerFromMap(Player* player, bool del)
{
	player->removeFromWorld();
	player->removeFromGrid();

	if(del)
		this->deleteFromWorld(player);
}

template<class T>
void BattleMap::addToMap(T* obj)
{
	GridCoord coord = computeGridCoordForPosition(obj->getData()->getPosition());
	this->ensureGridLoaded(coord);
	this->addToGrid(obj, coord);

	obj->setMap(this);
	obj->addToWorld();

	obj->updateObjectVisibility();
	obj->updateObjectTraceability();
	obj->updateObjectSafety();
}

template<class T>
void BattleMap::removeFromMap(T* obj, bool del)
{
	obj->removeFromWorld();
	obj->removeFromGrid();

	obj->setMap(nullptr);

	if (del)
	{
		m_reusableObjectsStore.remove(obj);
		deleteFromWorld(obj);
	}
	else
		m_reusableObjectsStore.insert(obj);
}

void BattleMap::prepareForBattle()
{
	int32 nFlags = (int32)(m_mapData->getMapSize().width * m_mapData->getMapSize().height);
	std::fill(m_tileFlagsSet, m_tileFlagsSet + nFlags, TILE_FLAG_NONE);

	m_splitOpenPoints = m_mapData->getSplitOpenPoints();
	m_splitUnconcealableOpenPoints = m_mapData->getSplitUnconcealableOpenPoints();

	if (!this->isTrainingGround())
	{
		m_isSafeZoneEnabled = true;
		this->setupSafeZone();
		this->initDistrictWaypointNodes();
		m_unitSpawnPointGenerator.reset(true);
	}
	else
	{
		m_isSafeZoneEnabled = false;
		m_unitSpawnPointGenerator.reset(false);
	}
}

bool BattleMap::canJoinBattle() const
{
	if (m_battleState == BATTLE_STATE_NONE)
		return true;

	return false;
}

NSTime BattleMap::getDurationByBattleState(BattleState state) const
{
	switch (state)
	{
	case BATTLE_STATE_PREPARING:
		return sWorld->getIntConfig(CONFIG_BATTLE_PREPARATION_DURATION);
	case BATTLE_STATE_IN_PROGRESS:
		return m_mapData->getBattleDuration() * 1000;
	default:
		return 0;
	}
}

void BattleMap::sendBattleUpdate(Player* player, uint32 updateFlags)
{
	if (!player->getSession())
		return;

	BattleUpdate update;
	update.set_update_flags(updateFlags);

	if ((updateFlags & BATTLE_UPDATEFLAG_STATE) != 0)
	{
		switch (m_battleState)
		{
		case BATTLE_STATE_PREPARING:
		{
			update.set_preparation_duration(m_battleStateTimer.getDuration());
			update.set_battle_duration(this->getDurationByBattleState(BATTLE_STATE_IN_PROGRESS));
			NSTime startTime = player->getSession()->getClientNowTimeMillis() - this->getBattleStateElapsedTime();
			update.set_start_time(startTime);
			break;
		}
		case BATTLE_STATE_IN_PROGRESS:
		{
			update.set_battle_duration(m_battleStateTimer.getDuration());
			NSTime startTime = player->getSession()->getClientNowTimeMillis() - this->getBattleStateElapsedTime();
			update.set_start_time(startTime);
			break;
		}
		default:
			break;
		}
		update.set_state(m_battleState);
	}

	if ((updateFlags & BATTLE_UPDATEFLAG_ALIVE_COUNT) != 0)
		update.set_alive_count(m_aliveCounter);

	if ((updateFlags & BATTLE_UPDATEFLAG_MAGICBEAN_COUNT) != 0)
		update.set_magicbean_count(m_spawnManager->getClassifiedItemCount(ITEM_CLASS_MAGIC_BEAN));

	WorldPacket packet(SMSG_BATTLE_UPDATE);
	player->getSession()->packAndSend(std::move(packet), update);
}

void BattleMap::sendBattleUpdateToPlayers(uint32 updateFlags)
{
	for (auto it = m_playerList.begin(); it != m_playerList.end(); ++it)
	{
		Player* player = (*it).second;
		this->sendBattleUpdate(player, updateFlags);
	}
}

bool BattleMap::isWithinCombatGrade(uint16 combatPower) const
{
	return combatPower >= m_combatGrade.minCombatPower && combatPower <= m_combatGrade.maxCombatPower;
}

void BattleMap::resetCombatGrade()
{
	m_combatGrade = {};
}

void BattleMap::setupSafeZone()
{
	TileCoord center;

#if USE_MAP_CENTER_AS_SAFEZONE_CENTER
	int32 mapWidth = (int32)m_mapData->getMapSize().width;
	int32 mapHeight = (int32)m_mapData->getMapSize().height;
	center.x = (int32)std::ceil(mapWidth * 0.5f) - 1;
	center.y = (int32)std::ceil(mapHeight * 0.5f) - 1;
	std::vector<TileCoord> result;
	this->findNearestOpenTileList(center, result, true);
	NS_ASSERT(!result.empty());
	center = result.front();

#else
	center = this->randomOpenTile(true);
#endif // USE_MAP_CENTER_AS_SAFEZONE_CENTER

	this->setSafeZoneCenter(center);
}

bool BattleMap::isWithinSafeZone(TileCoord const& tileCoord) const
{
	if (!m_isSafeZoneEnabled)
		return true;

	bool bRet = false;

	int32 minX = m_safeZoneCenter.x - m_currSafeZoneRadius;
	int32 minY = m_safeZoneCenter.y - m_currSafeZoneRadius;
	int32 maxX = m_safeZoneCenter.x + m_currSafeZoneRadius;
	int32 maxY = m_safeZoneCenter.y + m_currSafeZoneRadius;

	if (tileCoord.x > minX && tileCoord.x <= maxX
		&& tileCoord.y > minY && tileCoord.y <= maxY)
	{
		bRet = true;
	}


	return bRet;
}

bool BattleMap::isSafeDistanceMaintained(TileCoord const& tileCoord) const
{
	if (!m_isSafeZoneEnabled)
		return true;

	bool bRet = false;

	int32 side = m_currSafeZoneRadius - m_currSafeDistance;
	int32 minX = m_safeZoneCenter.x - side;
	int32 minY = m_safeZoneCenter.y - side;
	int32 maxX = m_safeZoneCenter.x + side;
	int32 maxY = m_safeZoneCenter.y + side;

	if (tileCoord.x > minX && tileCoord.x <= maxX
		&& tileCoord.y > minY && tileCoord.y <= maxY)
	{
		bRet = true;
	}

	return bRet;
}

int32 BattleMap::calcRadiusToSafeZoneCenter(TileCoord const& start)
{
	int32 diffX = start.x - m_safeZoneCenter.x;
	int32 diffY = start.y - m_safeZoneCenter.y;
	int32 radius = std::max(std::abs(diffX), std::abs(diffY)) + 1;
	if ((diffX + diffY) > 0)
		--radius;

	return radius;
}

void BattleMap::randomPatrolPoint(TileCoord const& currCoord, TileCoord& result) const
{
	auto currTime = getHighResolutionTimeMillis();

	int32 startIndex = 0;
	int32 radius = m_currSafeZoneRadius - m_currSafeDistance;
	auto it = m_patrolPointIndexes.find(radius);
	if (it != m_patrolPointIndexes.end())
		startIndex = (*it).second;

	int32 endIndex = (int32)m_patrolPointList.size() - 1;
	if (startIndex >= endIndex)
		startIndex = 0;
	int32 index = random(startIndex, endIndex);
	TileCoord newCoord = m_patrolPointList[index].second;
	if (newCoord == currCoord)
	{
		++index;
		if (index > endIndex)
			index = startIndex;
		newCoord = m_patrolPointList[index].second;
	}

	result = newCoord;

	auto elapsed = getHighResolutionTimeMillis() - currTime;
	NS_LOG_DEBUG("world.map", "randomPatrolPoint(result: %d,%d) time in %f ms", newCoord.x, newCoord.y, elapsed);

}

bool BattleMap::isWithinPatrolArea(TileCoord const& tileCoord) const
{
	bool bRet = false;

	int32 width = (int32)m_mapData->getMapSize().width;
	int32 height = (int32)m_mapData->getMapSize().height;
	int32 minX = std::max(0, m_safeZoneCenter.x - SIGHT_DISTANCE_IN_TILES + 1);
	int32 minY = std::max(0, m_safeZoneCenter.y - SIGHT_DISTANCE_IN_TILES + 1);
	int32 maxX = std::min(width - 1, m_safeZoneCenter.x + SIGHT_DISTANCE_IN_TILES);
	int32 maxY = std::min(height - 1, m_safeZoneCenter.y + SIGHT_DISTANCE_IN_TILES);

	if (tileCoord.x >= minX && tileCoord.x <= maxX
		&& tileCoord.y >= minY && tileCoord.y <= maxY)
	{
		NS_ASSERT(m_mapData->isValidTileCoord(tileCoord));
		if (m_mapData->isSameDistrict(tileCoord, m_safeZoneCenter))
		{
			bRet = true;
		}
	}

	return bRet;
}

bool BattleMap::checkPatrolConditions(TileCoord const& currCoord) const
{
	int32 diff = m_currSafeZoneRadius - m_currSafeDistance;
	if (diff <= SIGHT_DISTANCE_IN_TILES && this->isWithinPatrolArea(currCoord))
	{
		return true;
	}

	return false;
}

bool BattleMap::isDangerousDistrict(TileCoord const& coord) const
{
	return m_districtWaypointNodes.find(m_mapData->getDistrictId(coord)) == m_districtWaypointNodes.end();
}

WaypointNodeMap const* BattleMap::getDistrictWaypointNodes(TileCoord const& coord) const
{
	auto it = m_districtWaypointNodes.find(m_mapData->getDistrictId(coord));
	if (it != m_districtWaypointNodes.end())
		return &(*it).second;

	return nullptr;
}

int32 BattleMap::getRobotCount() const
{
	return (int32)m_objectsStore.count<Robot>();
}

int32 BattleMap::getItemCount() const
{
	return (int32)m_objectsStore.count<Item>();
}

void BattleMap::increaseAliveCounter()
{
	++m_aliveCounter;
	m_isAliveCountDirty = true;
}

void BattleMap::decreaseAliveCounter()
{
	NS_ASSERT(m_aliveCounter > 0);
	--m_aliveCounter;
	m_isAliveCountDirty = true;
}

void BattleMap::playerKilled(Player* player)
{
	this->decreaseAliveCounter();

	if (!this->isTrainingGround())
	{
		int32 rankNo = m_aliveCounter + 1;
		if (m_aliveCounter > 0)
			player->withdrawDelayed(BATTLE_DEFEAT, rankNo);
		else
			player->withdrawDelayed(BATTLE_VICTORY, rankNo);
	}
}

void BattleMap::robotKilled(Robot* robot)
{
	this->decreaseAliveCounter();

	int32 rankNo = m_aliveCounter + 1;
	if (m_aliveCounter > 0)
		robot->withdrawDelayed(BATTLE_DEFEAT, rankNo);
}

void BattleMap::increasePlayerInBattleCounter()
{
	++m_playerInBattleCounter;
}

void BattleMap::decreasePlayerInBattleCounter()
{
	NS_ASSERT(m_playerInBattleCounter > 0);
	--m_playerInBattleCounter;
}

void BattleMap::addObjectToRemoveList(WorldObject* obj)
{
	obj->cleanupBeforeDelete();
	m_objectsToRemove.insert(obj);
}

template<typename T>
T* BattleMap::takeReusableObject()
{
	T* obj = nullptr;
	auto& objects = m_reusableObjectsStore.getElement<T>();
	auto it = objects.begin();
	if (it != objects.end())
	{
		obj = *it;
		objects.erase(it);
	}

	return obj;
}

void BattleMap::addObjectToNewGrid(WorldObject* target)
{
	m_objectToNewGridList.push_back(target);
}

void BattleMap::playerRelocation(Player* player, Point const& newPosition)
{
	GridCoord oldCoord = computeGridCoordForPosition(player->getData()->getPosition());
	GridCoord newCoord = computeGridCoordForPosition(newPosition);

	if (oldCoord != newCoord)
	{
		this->ensureGridLoaded(newCoord);

		player->removeFromGrid();
		GridType* grid = this->getGrid(newCoord);
		NS_ASSERT(grid != nullptr);
		grid->addWorldObject(player);
		//NS_LOG_DEBUG("world.map", "GUID=%s [%d,%d]=>[%d,%d]", player->getData()->getGuid().toString().c_str(), oldCoord.x, oldCoord.y, newCoord.x, newCoord.y);
	}

	player->getData()->setPosition(newPosition);
	player->updateObjectVisibility();
	player->updateObjectTraceability();
	player->updateObjectSafety();
}

void BattleMap::robotRelocation(Robot* robot, Point const& newPosition)
{
	GridCoord oldCoord = computeGridCoordForPosition(robot->getData()->getPosition());
	GridCoord newCoord = computeGridCoordForPosition(newPosition);
	Point oldPosition = robot->getData()->getPosition();

	if (oldCoord != newCoord)
	{
		this->ensureGridLoaded(newCoord);

		robot->getData()->setPosition(newPosition);
		this->addObjectToNewGrid(robot);
	}
	else
	{
		robot->getData()->setPosition(newPosition);
		robot->updateObjectVisibility();
		robot->updateObjectTraceability();
		robot->updateObjectSafety();
	}
}

void BattleMap::itemRelocation(Item* item, Point const& newPosition)
{
	GridCoord oldCoord = computeGridCoordForPosition(item->getData()->getPosition());
	GridCoord newCoord = computeGridCoordForPosition(newPosition);
	Point oldPosition = item->getData()->getPosition();

	if (oldCoord != newCoord)
	{
		this->ensureGridLoaded(newCoord);

		item->getData()->setPosition(newPosition);
		this->addObjectToNewGrid(item);
	}
	else
	{
		item->getData()->setPosition(newPosition);
		item->updateObjectVisibility();
	}
}

void BattleMap::projectileRelocation(Projectile* proj, Point const& newPosition)
{
	GridCoord oldCoord = computeGridCoordForPosition(proj->getData()->getPosition());
	GridCoord newCoord = computeGridCoordForPosition(newPosition);
	Point oldPosition = proj->getData()->getPosition();

	if (oldCoord != newCoord)
	{
		this->ensureGridLoaded(newCoord);

		proj->getData()->setPosition(newPosition);
		this->addObjectToNewGrid(proj);
	}
	else
	{
		proj->getData()->setPosition(newPosition);
		proj->updateObjectVisibility();
	}
}

void BattleMap::sendObjectUpdates()
{
	PlayerUpdateMapType updateMap;

	// Store all players who need to receive update objects in updateMap
	for (auto it = m_updateObjects.begin(); it != m_updateObjects.end(); ++it)
	{
		Object* obj = *it;
		NS_ASSERT(obj->isInWorld());

		obj->buildValuesUpdate(updateMap);
	}

	for (auto it = updateMap.begin(); it != updateMap.end(); ++it)
	{
		Player* player = it->first;
		if (WorldSession* session = player->getSession())
		{
			WorldPacket packet(SMSG_UPDATE_OBJECT);
			UpdateObject& update = it->second;
			session->packAndSend(std::move(packet), update);
		}
	}

	// Clean up objects after sending updates
	while (!m_updateObjects.empty())
	{
		Object* obj = *m_updateObjects.begin();
		m_updateObjects.erase(m_updateObjects.begin());
		obj->cleanupAfterUpdate();
	}
}

void BattleMap::sendGlobalFlashMessage(FlashMessage const& message, Player* self)
{
	for (auto iter = m_playerList.begin(); iter != m_playerList.end(); ++iter)
	{
		Player* player = (*iter).second;
		if (player == self || !player->getSession())
			continue;

		player->getSession()->sendFlashMessage(message);
	}
}

void BattleMap::sendGlobalPlayerActionMessage(PlayerActionMessage const& message, Player* self)
{
	for (auto iter = m_playerList.begin(); iter != m_playerList.end(); ++iter)
	{
		Player* player = (*iter).second;
		if (player == self || !player->getSession())
			continue;

		player->getSession()->sendPlayerActionMessage(message);
	}
}

uint32 BattleMap::generateRobotSpawnId()
{
	++m_robotSpawnIdCounter;
	NS_ASSERT(m_robotSpawnIdCounter <= uint32(0xFFFFFF), "Robot spawn id overflow!");
	return m_robotSpawnIdCounter;
}

uint32 BattleMap::generateItemBoxSpawnId()
{
	++m_itemBoxSpawnIdCounter;
	NS_ASSERT(m_itemBoxSpawnIdCounter <= uint32(0xFFFFFF), "ItemBox spawn id overflow!");
	return m_itemBoxSpawnIdCounter;
}

uint32 BattleMap::generateItemSpawnId()
{
	++m_itemSpawnIdCounter;
	NS_ASSERT(m_itemSpawnIdCounter <= uint32(0xFFFFFF), "Item spawn id overflow!");
	return m_itemSpawnIdCounter;
}

uint32 BattleMap::generateProjectileSpawnId()
{
	++m_projectileSpawnIdCounter;
	NS_ASSERT(m_projectileSpawnIdCounter <= uint32(0xFFFFFF), "Projectile spawn id overflow!");
	return m_projectileSpawnIdCounter;
}

uint32 BattleMap::generateUnitLocatorSpawnId()
{
	++m_unitLocatorSpawnIdCounter;
	NS_ASSERT(m_unitLocatorSpawnIdCounter <= uint32(0xFFFFFF), "UnitLocator spawn id overflow!");
	return m_unitLocatorSpawnIdCounter;
}

void BattleMap::restart()
{
	m_precomputeMap->calculateMap();

	m_rankTotal = m_aliveCounter;
	m_isAliveCountDirty = false;
	m_isPatrolPointsDirty = true;

	if (!this->isTrainingGround())
		m_battleState = BATTLE_STATE_PREPARING;
	else
		m_battleState = BATTLE_STATE_IN_PROGRESS;

	m_battleStateTimer.setDuration(this->getDurationByBattleState(m_battleState));
	this->sendBattleUpdateToPlayers(BATTLE_UPDATEFLAG_ALL);
}

void BattleMap::updateSafeZone()
{
	if (!m_isSafeZoneEnabled)
		return;

	float scale = std::min(1.0f, m_battleStateTimer.getElapsed() / (float)m_battleStateTimer.getDuration());
	int32 newRadius = m_initialSafeZoneRadius - (int32)(m_initialSafeZoneRadius * scale);
	if (newRadius != m_currSafeZoneRadius)
	{
		while (m_currSafeZoneRadius > newRadius)
		{
			--m_currSafeZoneRadius;
			int32 diff = m_currSafeZoneRadius - m_currSafeDistance;
			if (diff < SIGHT_DISTANCE_IN_TILES)
			{
				if (m_currSafeZoneRadius % 2 != 0)
					--m_currSafeDistance;
			}
		}

		if (!m_isDangerAlertTriggered)
		{
			int32 mapWidth = (int32)m_mapData->getMapSize().width;
			int32 mapHeight = (int32)m_mapData->getMapSize().height;
			int32 top = m_safeZoneCenter.y;
			int32 bottom = mapHeight - m_safeZoneCenter.y - 1;
			int32 left = m_safeZoneCenter.x;
			int32 right = mapWidth - m_safeZoneCenter.x - 1;
			int32 alertRadius = std::max(right, std::max(left, std::max(top, bottom))) - (MAP_MARGIN_IN_TILES - 1);
			if (m_currSafeZoneRadius <= alertRadius)
				m_isDangerAlertTriggered = true;
		}

		NS_LOG_DEBUG("world.map", "currSafeZoneRadius: %d currSafeDistance: %d", m_currSafeZoneRadius, m_currSafeDistance);

		this->updateDistrictWaypointNodes();
		m_isSafeZoneRelocated = true;
	}
}

void BattleMap::setSafeZoneCenter(TileCoord const& center)
{
	m_safeZoneCenter = center;

	int32 mapWidth = (int32)m_mapData->getMapSize().width;
	int32 mapHeight = (int32)m_mapData->getMapSize().height;
	m_initialSafeZoneRadius = std::max(mapWidth, mapHeight);

	m_currSafeZoneRadius = m_initialSafeZoneRadius;
	m_currSafeDistance = SIGHT_DISTANCE_IN_TILES; // Safe distance from the danger zone

	NS_LOG_DEBUG("world.map", "safeZoneCenter: %d,%d currSafeZoneRadius: %d currSafeDistance: %d", m_safeZoneCenter.x, m_safeZoneCenter.y, m_initialSafeZoneRadius, m_currSafeDistance);
}

void BattleMap::updateBattleState(NSTime diff)
{
	if (m_battleState == BATTLE_STATE_ENDED)
		return;

	uint32 updateFlags = BATTLE_UPDATEFLAG_NONE;
	if (m_isAliveCountDirty)
	{
		updateFlags |= BATTLE_UPDATEFLAG_ALIVE_COUNT;
		m_isAliveCountDirty = false;
	}

	m_battleStateTimer.update(diff);
	switch (m_battleState)
	{
	case BATTLE_STATE_PREPARING:
		if (m_battleStateTimer.passed())
		{
			m_battleStateTimer.setDuration(this->getDurationByBattleState(BATTLE_STATE_IN_PROGRESS));
			m_battleState = BATTLE_STATE_IN_PROGRESS;
			updateFlags |= BATTLE_UPDATEFLAG_STATE;
		}
		break;
	case BATTLE_STATE_IN_PROGRESS:
		this->updateSafeZone();

		// When the population cap is set to 1, the battle will not change to the ending state
		if (m_mapData->getPopulationCap() > 1)
		{
			if (m_aliveCounter <= 1 && m_spawnManager->isAllPlayersHere())
			{
				if (!this->isTrainingGround()
					// Robots have been added to the training ground
					|| this->getRobotCount() > 0)
				{
					m_battleStateTimer.reset();
					m_battleState = BATTLE_STATE_ENDING;
					updateFlags |= BATTLE_UPDATEFLAG_STATE;

					if (!this->isTrainingGround())
					{
						if (Player* victor = this->findVictoriousPlayer())
							victor->withdrawDelayed(BATTLE_VICTORY, 1);
					}
				}
			}
		}
		break;
	case BATTLE_STATE_ENDING:
		if (m_playerInBattleCounter <= 0)
		{
			m_battleState = BATTLE_STATE_ENDED;
		}
	default:
		break;
	}

	if (updateFlags != BATTLE_UPDATEFLAG_NONE)
		this->sendBattleUpdateToPlayers(updateFlags);
}

Player* BattleMap::findVictoriousPlayer() const
{
	for (auto it = m_playerList.begin(); it != m_playerList.end(); ++it)
	{
		Player* player = (*it).second;
		if (player->isAlive())
			return player;
	}

	return nullptr;
}

template<typename T>
void BattleMap::addToGrid(T* obj, GridCoord const& coord)
{
	// Add the obj to the grid container specified by the coord
	this->getGrid(coord)->addWorldObject(obj);
}

void BattleMap::setTileClosedPosition(Point const& position, bool isClosed)
{
	TileCoord tileCoord(m_mapData->getMapSize(), position);
	this->setTileClosed(tileCoord, isClosed);
}

void BattleMap::setTileClosed(TileCoord const& coord, bool isClosed)
{
	if (isClosed)
		this->markClosedTileFlag(coord);
	else
		this->clearClosedTileFlag(coord);
}

void BattleMap::setTileClosedToNewPosition(Point const& oldPosition, Point const& newPosition)
{
	TileCoord oldTileCoord(m_mapData->getMapSize(), oldPosition);
	TileCoord newTileCoord(m_mapData->getMapSize(), newPosition);

	if (oldTileCoord != newTileCoord)
	{
		this->clearClosedTileFlag(oldTileCoord);
		this->markClosedTileFlag(newTileCoord);
	}
}

void BattleMap::setTileAreaClosed(TileArea const& tileArea, bool isClosed)
{
	TileCoord coord;
	for (int32 x = tileArea.lowBound.x; x <= tileArea.highBound.x; ++x)
	{
		for (int32 y = tileArea.lowBound.y; y <= tileArea.highBound.y; ++y)
		{
			coord.x = x;
			coord.y = y;
			this->setTileClosed(coord, isClosed);
		}
	}
}


void BattleMap::setTileFlags(TileCoord const& coord, uint16 flags)
{
	m_tileFlagsSet[m_mapData->getTileIndex(coord)] = flags;
}

void BattleMap::addTileFlag(TileCoord const& coord, uint16 flag)
{
	if (!this->hasTileFlag(coord.x, coord.y, flag))
		m_tileFlagsSet[m_mapData->getTileIndex(coord)] |= flag;
}

bool BattleMap::hasTileFlag(int32 x, int32 y, uint16 flag) const
{
	return (m_tileFlagsSet[m_mapData->getTileIndex(x, y)] & flag) != 0;
}

void BattleMap::clearTileFlag(TileCoord const& coord, uint16 flag)
{
	if (hasTileFlag(coord, flag))
		m_tileFlagsSet[m_mapData->getTileIndex(coord)] &= ~flag;
}

uint16 BattleMap::getTileFlags(TileCoord const& coord) const
{
	return m_tileFlagsSet[m_mapData->getTileIndex(coord)];
}

void BattleMap::findNearestOpenTile(TileCoord const& findCoord, TileCoord& result, bool isExcludeHidingSpots) const
{
	std::vector<TileCoord> foundCoordList;
	this->findNearestOpenTileList(findCoord, foundCoordList, isExcludeHidingSpots);
	NS_ASSERT(!foundCoordList.empty());
	int32 index = 0;
	if (foundCoordList.size() > 1)
		index = random(0, (int32)(foundCoordList.size() - 1));
	result = foundCoordList[index];
}

void BattleMap::findNearestOpenTile(TileCoord const& findCoord, TileCoord const& closestToCoord, TileCoord& result, bool isExcludeHidingSpots) const
{
	std::vector<TileCoord> foundCoordList;
	this->findNearestOpenTileList(findCoord, foundCoordList, isExcludeHidingSpots);
	NS_ASSERT(!foundCoordList.empty());
	// Sort to find the position nearest to currCoord
	if (foundCoordList.size() > 1)
	{
		std::multimap<int32, std::size_t> orderedCoords;
		for (std::size_t i = 0; i < foundCoordList.size(); ++i)
		{
			auto const& coord = foundCoordList[i];
			int32 dx = coord.x - closestToCoord.x;
			int32 dy = coord.y - closestToCoord.y;
			orderedCoords.emplace((dx * dx) + (dy * dy), i);
		}
		auto it = orderedCoords.begin();
		NS_ASSERT(it != orderedCoords.end());
		result = foundCoordList[(*it).second];
	}
	else
		result = foundCoordList.front();
}

void BattleMap::findNearestOpenTileList(TileCoord const& findCoord, std::vector<TileCoord>& result, bool isExcludeHidingSpots) const
{
	auto currTime = getHighResolutionTimeMillis();

	TileCoord vaildCoord;
	if (!m_mapData->isValidTileCoord(findCoord))
	{
		int32 width = (int32)m_mapData->getMapSize().width;
		int32 height = (int32)m_mapData->getMapSize().height;
		vaildCoord.x = std::min(std::max(findCoord.x, 0), width - 1);
		vaildCoord.y = std::min(std::max(findCoord.y, 0), height - 1);
	}
	else
		vaildCoord = findCoord;
	if (!m_mapData->isWall(vaildCoord) && (!isExcludeHidingSpots || !m_mapData->isConcealable(vaildCoord)) && !this->isTileClosed(vaildCoord))
		result.emplace_back(vaildCoord);
	else
	{
		bool found = false;
		if (m_mapData->isWall(vaildCoord) || (isExcludeHidingSpots && m_mapData->isConcealable(vaildCoord)))
		{
			std::vector<TileCoord> foundCoordList;
			m_mapData->findNearestOpenPointList(vaildCoord, foundCoordList, isExcludeHidingSpots);
			for (auto it = foundCoordList.begin(); it != foundCoordList.end();)
			{
				TileCoord const& coord = *it;
				if (this->isTileClosed(coord))
					it = foundCoordList.erase(it);
				else
					++it;
			}
			if (!foundCoordList.empty())
			{
				result.insert(result.end(), foundCoordList.begin(), foundCoordList.end());
				found = true;
			}
		}

		if (!found)
			m_mapData->findNearestOpenPointList(isExcludeHidingSpots ? m_splitUnconcealableOpenPoints : m_splitOpenPoints, vaildCoord, result);
	}

	auto elapsed = getHighResolutionTimeMillis() - currTime;
	//NS_LOG_DEBUG("world.map", "findNearestOpenTileList() time in %f ms", elapsed);
}

TileCoord BattleMap::randomOpenTile(bool isExcludeHidingSpots) const
{
	TileArea area;
	area.lowBound.x = 0;
	area.lowBound.y = 0;
	area.highBound.x = (int32)m_mapData->getMapSize().width - 1;
	area.highBound.y = (int32)m_mapData->getMapSize().height - 1;
	return this->randomOpenTileInArea(area);
}

TileCoord BattleMap::randomOpenTileInArea(TileArea const& area, bool isExcludeHidingSpots) const
{
	TileCoord ranCoord;
	int32 minX = std::max(area.lowBound.x, MAP_MARGIN_IN_TILES);
	int32 maxX = std::min((int32)m_mapData->getMapSize().width - MAP_MARGIN_IN_TILES - 1, area.highBound.x);
	int32 minY = std::max(area.lowBound.y, MAP_MARGIN_IN_TILES);
	int32 maxY = std::min((int32)m_mapData->getMapSize().height - MAP_MARGIN_IN_TILES - 1, area.highBound.y);
	ranCoord.x = random(minX, maxX);
	ranCoord.y = random(minY, maxY);

	this->findNearestOpenTile(ranCoord, ranCoord, isExcludeHidingSpots);

	return ranCoord;
}

TileCoord BattleMap::randomOpenTileOnCircle(TileCoord const& center, int32 radius, bool isExcludeHidingSpots) const
{
	TileCoord ranCoord;
	// https://programming.guide/random-point-within-circle.html
	float r = radius * sqrt(random(0.f, 1.0f));
	float a = random(0.f, 1.0f) * 2 * (float)M_PI;
	float x = center.x + r * std::cos(a);
	float y = center.y + r * std::sin(a);
	ranCoord.x = (int32)std::round(x);
	ranCoord.y = (int32)std::round(y);

	this->findNearestOpenTile(ranCoord, ranCoord, isExcludeHidingSpots);

	return ranCoord;
}

TileCoord BattleMap::generateUnitSpawnPoint()
{
	TileCoord spawnPoint = m_unitSpawnPointGenerator.nextPosition();
	return spawnPoint;
}

void BattleMap::initGrids()
{
	for (int i = 0; i < MAX_NUMBER_OF_GRIDS; i++)
	{
		for (int j = 0; j < MAX_NUMBER_OF_GRIDS; j++)
		{
			m_grids[i][j] = nullptr;
		}
	}
}

void BattleMap::unloadAllGrids()
{
	for (int i = 0; i < MAX_NUMBER_OF_GRIDS; i++)
	{
		for (int j = 0; j < MAX_NUMBER_OF_GRIDS; j++)
		{
			GridType* grid = m_grids[i][j];
			if (grid)
			{
				this->unloadGrid(grid);
			}
		}
	}
}

void BattleMap::unloadGrid(GridType* grid)
{
	GridCoord gridCoord = grid->getGridCoord();

	ObjectGridUnloader unloader(this);
	TypeContainerVisitor<ObjectGridUnloader, WorldTypeGridContainer> visitor(unloader);
	grid->visit(visitor);

	m_gridRefList.remove(grid);
	this->setGrid(gridCoord, nullptr);

	delete grid;
}

void BattleMap::cleanupAllGrids()
{
	for (int i = 0; i < MAX_NUMBER_OF_GRIDS; i++)
	{
		for (int j = 0; j < MAX_NUMBER_OF_GRIDS; j++)
		{
			GridType* grid = m_grids[i][j];
			if (grid)
			{
				this->cleanupGrid(grid);
			}
		}
	}
}

void BattleMap::cleanupGrid(GridType* grid)
{
	GridCoord gridCoord = grid->getGridCoord();

	ObjectGridCleaner cleaner(this);
	TypeContainerVisitor<ObjectGridCleaner, WorldTypeGridContainer> visitor(cleaner);
	grid->visit(visitor);
	grid->setGridObjectDataLoaded(false);
}

void BattleMap::ensureGridLoaded(GridCoord const& coord)
{
	this->ensureGridCreated(coord);

	GridType* grid = this->getGrid(coord);
	if (!grid->isGridObjectDataLoaded())
	{
		ObjectGridLoader loader(this, coord);
		TypeContainerVisitor<ObjectGridLoader, WorldTypeGridContainer> visitor(loader);

		grid->visit(visitor);
		grid->setGridObjectDataLoaded(true);
	}
}

void BattleMap::ensureGridCreated(GridCoord const& coord)
{
	// If it has not been created, create the grid
	if (!this->getGrid(coord))
	{
		GridType* grid = new GridType(coord);
		this->setGrid(coord, grid);
		m_gridRefList.push_back(grid);

		//NS_LOG_DEBUG("world.map", "GRID[%d,%d] Created", coord.x, coord.y);
	}
}

void BattleMap::clearClosedTileFlag(TileCoord const& coord)
{
	if (this->hasTileFlag(coord, TILE_FLAG_CLOSED))
	{
		NS_ASSERT(m_mapData->isValidTileCoord(coord) && !m_mapData->isWall(coord));
		this->clearTileFlag(coord, TILE_FLAG_CLOSED);

		int32 xMinusY = coord.x - coord.y;
		int32 xPlusY = coord.x + coord.y;
		m_splitOpenPoints[xMinusY][xPlusY] = coord;
		if(!m_mapData->isConcealable(coord))
			m_splitUnconcealableOpenPoints[xMinusY][xPlusY] = coord;

		m_precomputeMap->invalidate();
		if (this->isWithinPatrolArea(coord))
			m_isPatrolPointsDirty = true;
		//NS_LOG_DEBUG("world.map", "CLEAR CLOSED TILEFLAG[%d, %d]", coord.x, coord.y);
	}
}

void BattleMap::markClosedTileFlag(TileCoord const& coord)
{
	if (m_mapData->isValidTileCoord(coord) && !m_mapData->isWall(coord)
		&& !this->hasTileFlag(coord, TILE_FLAG_CLOSED))
	{
		this->addTileFlag(coord, TILE_FLAG_CLOSED);

		int32 xMinusY = coord.x - coord.y;
		int32 xPlusY = coord.x + coord.y;
		auto nRemoved = m_splitOpenPoints[xMinusY].erase(xPlusY);
		NS_ASSERT(nRemoved > 0);
		if (m_splitOpenPoints[xMinusY].empty())
			m_splitOpenPoints.erase(xMinusY);
		if (!m_mapData->isConcealable(coord))
		{
			auto nRemoved = m_splitUnconcealableOpenPoints[xMinusY].erase(xPlusY);
			NS_ASSERT(nRemoved > 0);
			if (m_splitUnconcealableOpenPoints[xMinusY].empty())
				m_splitUnconcealableOpenPoints.erase(xMinusY);
		}

		m_precomputeMap->invalidate();
		if (this->isWithinPatrolArea(coord))
			m_isPatrolPointsDirty = true;
		//NS_LOG_DEBUG("world.map", "MARK CLOSED TILEFLAG[%d, %d]", coord.x, coord.y);
	}
}

void BattleMap::processObjectToNewGridList()
{
	for (auto it = m_objectToNewGridList.begin(); it != m_objectToNewGridList.end(); ++it)
	{
		WorldObject* object = *it;
		GridCoord newCoord = computeGridCoordForPosition(object->getData()->getPosition());

		GridType* grid = this->getGrid(newCoord);
		NS_ASSERT(grid != nullptr);
		switch (object->getTypeID())
		{
		case TYPEID_ROBOT:
			object->asRobot()->removeFromGrid();
			grid->addWorldObject(object->asRobot());
			object->updateObjectVisibility();
			object->updateObjectTraceability();
			object->updateObjectSafety();
			break;
		case TYPEID_ITEM:
			object->asItem()->removeFromGrid();
			grid->addWorldObject(object->asItem());
			object->updateObjectVisibility();
			break;
		case TYPEID_PROJECTILE:
			object->asProjectile()->removeFromGrid();
			grid->addWorldObject(object->asProjectile());
			object->updateObjectVisibility();
			break;
		default:
			NS_LOG_ERROR("world.map", "Non-grid object (TypeId: %u) is in grid object move list, ignored.", object->getTypeID());
			break;
		}

		// NS_LOG_DEBUG("world.map", "GUID=%s to GRID[%d,%d]", object->getData()->getGuid().toString().c_str(), newCoord.x, newCoord.y);
	}

	m_objectToNewGridList.clear();
}

void BattleMap::updateObjectsInGridArea(GridArea const& area, TypeContainerVisitor<ObjectUpdater, WorldTypeGridContainer>& visitor)
{
	for (uint32 x = area.lowBound.x; x <= area.highBound.x; ++x)
	{
		for (uint32 y = area.lowBound.y; y <= area.highBound.y; ++y)
		{
			// Mark grid as visited to avoid visiting the same grid repeatedly
			GridCoord coord(x, y);
			if (isGridMarked(coord))
				continue;

			markGrid(coord);
			this->visit(coord, visitor, false);
		}
	}
}

void BattleMap::processRelocationNotifies(NSTime diff)
{
	if (m_gridRefList.empty())
		return;

	// Handle notifications of changes in the position of objects in the active state grid
	ObjectRelocation relocation(*this);
	TypeContainerVisitor<ObjectRelocation, WorldTypeGridContainer> relocationVisitor(relocation);
	for (auto it = m_gridRefList.begin(); it != m_gridRefList.end(); ++it)
	{
		GridType* grid = *it;
		// Only visit marked grids
		if (!isGridMarked(grid->getGridCoord()))
			continue;

		grid->visit(relocationVisitor);
	}

	if (m_isSafeZoneRelocated)
	{
		// Notify objects in the active state grid when the location of the safe zone changes
		SafeZoneRelocationNotifier notifier(*this);
		TypeContainerVisitor<SafeZoneRelocationNotifier, WorldTypeGridContainer> notifierVisitor(notifier);
		for (auto it = m_gridRefList.begin(); it != m_gridRefList.end(); ++it)
		{
			GridType* grid = *it;
			if (!isGridMarked(grid->getGridCoord()))
				continue;

			grid->visit(notifierVisitor);
		}
		m_isSafeZoneRelocated = false;
	}

	// Cleans the notification flags for objects
	ObjectNotifyCleaner cleaner;
	TypeContainerVisitor<ObjectNotifyCleaner, WorldTypeGridContainer> cleanerVisitor(cleaner);
	for (auto it = m_gridRefList.begin(); it != m_gridRefList.end(); ++it)
	{
		GridType* grid = *it;
		if (!isGridMarked(grid->getGridCoord()))
			continue;

		grid->visit(cleanerVisitor);
	}

}

void BattleMap::processObjectRemoveList()
{
	while (!m_objectsToRemove.empty())
	{
		auto itr = m_objectsToRemove.begin();
		WorldObject* obj = *itr;

		switch (obj->getTypeID())
		{
		case TypeID::TYPEID_ROBOT:
			this->removeFromMap(obj->asRobot(), false);
			break;
		case TypeID::TYPEID_ITEMBOX:
			this->removeFromMap(obj->asItemBox(), false);
			break;
		case TypeID::TYPEID_ITEM:
			this->removeFromMap(obj->asItem(), false);
			break;
		case TypeID::TYPEID_PROJECTILE:
			this->removeFromMap(obj->asProjectile(), false);
			break;
		default:
			NS_LOG_ERROR("world.map", "Non-grid object (TypeId: %u) is in grid object remove list, ignored.", obj->getTypeID());
			break;
		}

		m_objectsToRemove.erase(itr);
	}
}

void BattleMap::cleanupReusableObjects()
{
	ReusableObjectsStoreCleaner cleaner;
	TypeContainerVisitor<ReusableObjectsStoreCleaner, MapStoredObjectSetContainer> visitor(cleaner);
	visitor.visit(m_reusableObjectsStore);
}

void BattleMap::initWaypointNodes()
{
	auto currTime = getHighResolutionTimeMillis();

	int32 width = (int32)m_mapData->getMapSize().width;
	int32 height = (int32)m_mapData->getMapSize().height;

	auto const& districtWaypoints = m_mapData->getDistrictWaypoints();
	for (auto it = districtWaypoints.begin(); it != districtWaypoints.end(); ++it)
	{
		auto const& waypoints = (*it).second;
		for (TileCoord const& point : waypoints)
		{
			WaypointNode* node = new WaypointNode(point);
			int32 tileIndex = point.x + point.y * width;
			m_waypointNodes.emplace(tileIndex, node);
		}
	}

	auto elapsed = getHighResolutionTimeMillis() - currTime;
	NS_LOG_DEBUG("world.map", "initWaypointNodes() time in %f ms", elapsed);
}

void BattleMap::deleteWaypointNodes()
{
	for (auto it = m_waypointNodes.begin(); it != m_waypointNodes.end();)
	{
		WaypointNode* node = (*it).second;
		it = m_waypointNodes.erase(it);
		delete node;
	}
}

void BattleMap::initDistrictWaypointNodes()
{
	auto currTime = getHighResolutionTimeMillis();

	m_radialWaypointNodes.clear();
	m_districtWaypointNodes.clear();
	m_startWaypointNode = nullptr;

	int32 width = (int32)m_mapData->getMapSize().width;
	int32 height = (int32)m_mapData->getMapSize().height;

	auto const& districtWaypoints = m_mapData->getDistrictWaypoints();
	for (auto it = districtWaypoints.begin(); it != districtWaypoints.end(); ++it)
	{
		auto districtId = (*it).first;
		auto const& waypoints = (*it).second;
		for (TileCoord const& point : waypoints)
		{
			int32 tileIndex = point.x + point.y * width;
			WaypointNode* node = m_waypointNodes.at(tileIndex);
			node->unlink();
			auto ret = m_districtWaypointNodes[districtId].emplace(tileIndex, node);
			NS_ASSERT(ret.second);
		}
	}

	std::unordered_map<int32, int32> maxRadii;
	for (auto it = m_waypointNodes.begin(); it != m_waypointNodes.end(); ++it)
	{
		WaypointNode* sourceNode = (*it).second;
		int32 sourceIndex = sourceNode->getPosition().x + sourceNode->getPosition().y * width;

		TileCoord target;
		bool ret = m_mapData->getLinkedWaypoint(sourceNode->getPosition(), target);
		NS_ASSERT(ret);
		int32 targetIndex = target.x + target.y * width;
		WaypointNode* targetNode = m_waypointNodes.at(targetIndex);

		int32 minRadius = this->getMinRadiusWaypointToSafeZoneCenter(sourceNode->getPosition());
		int32 compositeId = sourceIndex + (width * height + targetIndex);
		auto radiusIt = maxRadii.find(compositeId);
		if (radiusIt == maxRadii.end())
			maxRadii.emplace(compositeId, minRadius);
		else
		{
			if (minRadius > (*radiusIt).second)
				m_radialWaypointNodes.emplace(minRadius, sourceNode);
			else
				m_radialWaypointNodes.emplace((*radiusIt).second, targetNode);
		}

		// Waypoint node graph
		sourceNode->link(targetNode);
		auto districtId = m_mapData->getDistrictId(target);
		auto const& waypoints = districtWaypoints.at(districtId);
		for (auto const& point : waypoints)
		{
			if (point == target)
				continue;

			int32 tileIndex = point.x + point.y * width;
			WaypointNode* node = m_waypointNodes.at(tileIndex);
			targetNode->link(node);
		}
	}

	auto districtId = m_mapData->getDistrictId(m_safeZoneCenter);
	auto it = districtWaypoints.find(districtId);
	if (it != districtWaypoints.end())
	{
		auto const& waypoints = (*it).second;
		for (auto const& point : waypoints)
		{
			int32 tileIndex = point.x + point.y * width;
			WaypointNode* node = m_waypointNodes.at(tileIndex);
			if (!m_startWaypointNode)
				m_startWaypointNode = node;
			else
			{
				int32 r1 = this->calcRadiusToSafeZoneCenter(point);
				int32 r2 = this->calcRadiusToSafeZoneCenter(m_startWaypointNode->getPosition());
				if (r1 < r2)
					m_startWaypointNode = node;
			}
		}
	}

	auto elapsed = getHighResolutionTimeMillis() - currTime;
	NS_LOG_DEBUG("world.map", "initDistrictWaypointNodes() time in %f ms", elapsed);
}

void BattleMap::updateDistrictWaypointNodes()
{
	auto currTime = getHighResolutionTimeMillis();

	int32 radius = m_currSafeZoneRadius - m_currSafeDistance;
	auto it = m_radialWaypointNodes.upper_bound(radius);
	for (; it != m_radialWaypointNodes.end();)
	{
		WaypointNode* node = (*it).second;
		if (!node->isIsolated())
		{
			node->unlink();
			TileCoord target;
			if (m_mapData->getLinkedWaypoint(node->getPosition(), target))
			{
				int32 tileIndex = target.x + target.y * (int32)m_mapData->getMapSize().width;
				uint32 districtId = m_mapData->getDistrictId(target);
				WaypointNode* targetNode = m_districtWaypointNodes.at(districtId).at(tileIndex);
				targetNode->unlink();
				NS_LOG_DEBUG("world.map", "Waypoints (%d,%d linkto %d,%d) removed. radius: %d", node->getPosition().x, node->getPosition().y, targetNode->getPosition().x, targetNode->getPosition().y, (*it).first);
			}
		}
		it = m_radialWaypointNodes.erase(it);
	}

	std::unordered_set<WaypointNode*> visitedNodes;
	if (m_startWaypointNode && !m_startWaypointNode->isIsolated())
		this->traverseLinkedWaypointNodes(visitedNodes, m_startWaypointNode);

	for (auto it = m_districtWaypointNodes.begin(); it != m_districtWaypointNodes.end(); )
	{
		uint32 districtId = (*it).first;
		auto& nodes = (*it).second;
		for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); )
		{
			WaypointNode* node = (*nodeIt).second;
			if (visitedNodes.find(node) == visitedNodes.end())
			{
				node->unlink();
				nodeIt = nodes.erase(nodeIt);
				NS_LOG_DEBUG("world.map", "Remove waypoint (%d,%d) from graph.", node->getPosition().x, node->getPosition().y);
			}
			else
				++nodeIt;
		}
		if (nodes.empty()
			&& districtId != m_mapData->getDistrictId(m_safeZoneCenter))
		{
			it = m_districtWaypointNodes.erase(it);
			// Cleans the district counter for objects.
			ObjectDistrictCounterCleaner cleaner(districtId);
			this->visitCreatedGrids(cleaner);
		}
		else
			++it;
	}

	auto elapsed = getHighResolutionTimeMillis() - currTime;
	NS_LOG_DEBUG("world.map", "updateDistrictWaypointNodes() time in %f ms", elapsed);
}

void BattleMap::traverseLinkedWaypointNodes(std::unordered_set<WaypointNode*>& visitedNodes, WaypointNode* startNode)
{
	//NS_LOG_DEBUG("world.map", "WaypointNode: %d,%d", startNode->position.x, startNode->position.y);
	visitedNodes.insert(startNode);
	for (auto node : startNode->getNextNodes())
	{
		if (visitedNodes.find(node) == visitedNodes.end())
			traverseLinkedWaypointNodes(visitedNodes, node);
	}
}

int32 BattleMap::getMinRadiusWaypointToSafeZoneCenter(TileCoord const& waypoint)
{
	auto const& waypointExtent = m_mapData->getWaypointExtent(waypoint);
	int32 minRadius = -1;
	for (auto const& point : waypointExtent)
	{
		int32 radius = this->calcRadiusToSafeZoneCenter(point);
		if (minRadius == -1 || radius < minRadius)
			minRadius = radius;
	}

	return minRadius;
}

void BattleMap::updatePatrolPoints()
{
	if (!m_isPatrolPointsDirty)
		return;

	auto currTime = getHighResolutionTimeMillis();

	m_patrolPointIndexes.clear();
	m_patrolPointList.clear();

	int32 width = (int32)m_mapData->getMapSize().width;
	int32 height = (int32)m_mapData->getMapSize().height;
	int32 minX = std::max(0, m_safeZoneCenter.x - SIGHT_DISTANCE_IN_TILES + 1);
	int32 minY = std::max(0, m_safeZoneCenter.y - SIGHT_DISTANCE_IN_TILES + 1);
	int32 maxX = std::min(width - 1, m_safeZoneCenter.x + SIGHT_DISTANCE_IN_TILES);
	int32 maxY = std::min(height - 1, m_safeZoneCenter.y + SIGHT_DISTANCE_IN_TILES);

	std::multimap<int32, TileCoord, std::greater<int32>> sightPoints;
	TileCoord coord;
	for (int32 x = minX; x <= maxX; ++x)
	{
		for (int32 y = minY; y <= maxY; ++y)
		{
			coord.x = x;
			coord.y = y;
			NS_ASSERT(m_mapData->isValidTileCoord(coord));
			if (!m_mapData->isWall(coord) && !this->isTileClosed(coord))
			{
				if (m_mapData->isSameDistrict(coord, m_safeZoneCenter))
				{
					int32 radius = this->calcRadiusToSafeZoneCenter(coord);
					sightPoints.emplace(radius, coord);
				}
			}
		}
	}

	for (auto pointIt = sightPoints.begin(); pointIt != sightPoints.end(); ++pointIt)
	{
		int32 radius = (*pointIt).first;
		TileCoord const& point = (*pointIt).second;
		m_patrolPointList.push_back(std::make_pair(radius, point));
		auto it = m_patrolPointIndexes.find(radius);
		if (it == m_patrolPointIndexes.end())
			m_patrolPointIndexes.emplace(radius, (int32)m_patrolPointList.size() - 1);
	}

	m_isPatrolPointsDirty = false;

	auto elapsed = getHighResolutionTimeMillis() - currTime;
	NS_LOG_DEBUG("world.map", "updatePatrolPoints() time in %f ms", elapsed);
}

void BattleMap::initJPSPlus()
{
	auto currTime = getHighResolutionTimeMillis();
	m_precomputeMap = new PrecomputeMap(this);
	m_jpsPlus = new JPSPlus(m_precomputeMap);

	auto elapsed = getHighResolutionTimeMillis() - currTime;
	NS_LOG_DEBUG("world.map", "initJPSPlus() time in %f ms", elapsed);
}

void BattleMap::resetMaxVisibleRange()
{
	m_maxVisibleRange.setSize(0, 0);
}

void BattleMap::updateMaxVisibleRangeWithPlayer(Player* player)
{
	Size visRange = player->getData()->getVisibleRange();
	m_maxVisibleRange.setSize(std::max(visRange.width + MAX_STEP_LENGTH * 4, m_maxVisibleRange.width),
		std::max(visRange.height + MAX_STEP_LENGTH * 4, m_maxVisibleRange.height));
}

template void BattleMap::addToMap<Robot>(Robot*);
template void BattleMap::removeFromMap<Robot>(Robot*, bool);
template void BattleMap::addToMap<ItemBox>(ItemBox*);
template void BattleMap::removeFromMap<ItemBox>(ItemBox*, bool);
template void BattleMap::addToMap<Item>(Item*);
template void BattleMap::removeFromMap<Item>(Item*, bool);
template void BattleMap::addToMap<Projectile>(Projectile*);
template void BattleMap::removeFromMap<Projectile>(Projectile*, bool);

template Robot* BattleMap::takeReusableObject<Robot>();
template ItemBox* BattleMap::takeReusableObject<ItemBox>();
template Item* BattleMap::takeReusableObject<Item>();
template Projectile* BattleMap::takeReusableObject<Projectile>();