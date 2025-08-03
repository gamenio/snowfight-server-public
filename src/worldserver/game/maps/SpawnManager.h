#ifndef __SPAWN_MANAGER_H__
#define __SPAWN_MANAGER_H__

#include "utilities/Timer.h"
#include "game/grids/GridDefines.h"
#include "game/dynamic/TypeContainer.h"
#include "game/entities/DataRobot.h"
#include "game/entities/DataItemBox.h"

struct SimpleRobot;
struct SimpleItemBox;
struct SimpleItem;
struct ItemBoxItem;

class Theater;
class BattleMap;

typedef std::vector<SimpleItemBox> SimpleItemBoxList;
typedef std::unordered_map<uint32/* GridId */, SimpleItemBoxList> SimpleItemBoxListContainer;
typedef std::vector<ItemBoxItem> ItemBoxItemList;
typedef std::unordered_map<uint32/* LootId */, ItemBoxItemList> ItemBoxItemListContainer;

typedef std::vector<SimpleItem> SimpleItemList;
typedef std::unordered_map<uint32/* GridId */, SimpleItemList> SimpleItemListContainer;
typedef std::map<uint32 /* ItemId */, uint32 /* Counter */> ItemCounterMap;
typedef std::map<uint32 /* ItemClass */, uint32 /* Counter */> ClassifiedItemCounterMap;

class SpawnManager
{
public:
	SpawnManager();
	~SpawnManager();

	void advancedUpdate(NSTime diff);
	void update(NSTime diff);

	void setTheater(Theater* theater) { m_theater = theater; }
	Theater* getTheater() const { return m_theater; }

	void setMap(BattleMap* map) { m_map = map; }
	BattleMap* getMap() const { return m_map; }

	bool addPlayerToQueue(Player* player);
	void removePlayerFromQueue(Player* player);

	void onPlayersInPlace();
	int32 getPlayersInPlaceCount() const { return (int32)m_pendingPlayers.size(); }
	bool isAllPlayersHere() const;
	void reset();

	SimpleItemBoxList const* getSimpleItemBoxList(uint32 gridId) const;
	ItemBoxItemList const* getItemBoxItemList(uint32 lootId) const;

	SimpleItemList const* getSimpleItemList(uint32 gridId) const;

	uint32 getItemCount(uint32 itemId) const;
	uint32 getClassifiedItemCount(uint32 itemClass) const;
	void increaseItemCount(uint32 itemId, int32 count);
	void increaseClassifiedItemCount(uint32 itemClass, int32 count);

	bool fillRobotsIfNeeded();

private:
	void configureSimpleRobot(SimpleRobot& simpleRobot);
	void spawnRobots(int32 numOfRobots);

	std::string generateRobotCountry();
	uint8 generateRobotLevel(RobotSpawnInfo const& spawnInfo, uint8 proficiencyLevel);
	std::string generateRobotName(std::string const& country);
	RobotDifficulty const& selectRobotDifficulty();
	RobotSpawnInfo const& selectRobotSpawnInfo();

	void prepareItemBoxs();
	void addSimpleItemBoxToGrid(SimpleItemBox const& simpleItemBox);
	void rollItemBoxLoot(ItemBoxTemplate const* itemBoxTemplate, uint32 lootId);
	void createItemBoxItem(uint32 lootId, ItemTemplate const* tmpl, int32 count);

	void prepareItems();
	void addSimpleItemToGrid(SimpleItem const& simpleItem);

	void processPendingPlayers();

	Theater* m_theater;
	BattleMap* m_map;
	std::list<Player*> m_pendingPlayers;

	SimpleItemBoxListContainer m_simpleItemBoxListStore;
	ItemBoxItemListContainer m_itemBoxItemListStore;

	SimpleItemListContainer m_simpleItemListStore;

	ItemCounterMap m_itemCounters;
	ClassifiedItemCounterMap m_classifiedItemCounters;
};


#endif // __SPAWN_MANAGER_H__
