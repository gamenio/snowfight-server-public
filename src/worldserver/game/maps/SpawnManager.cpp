#include "SpawnManager.h"

#include "utilities/Random.h"
#include "game/utils/MathTools.h"
#include "game/behaviors/Player.h"
#include "game/behaviors/Robot.h"
#include "game/behaviors/ItemBox.h"
#include "game/world/ObjectMgr.h"
#include "game/world/World.h"
#include "game/theater/Theater.h"

SpawnManager::SpawnManager() :
	m_theater(nullptr),
	m_map(nullptr)
{
}


SpawnManager::~SpawnManager()
{
	m_theater = nullptr;
	m_map = nullptr;
}

void SpawnManager::advancedUpdate(NSTime diff)
{
}

void SpawnManager::update(NSTime diff)
{
}

bool SpawnManager::addPlayerToQueue(Player* player)
{
	auto it = std::find(m_pendingPlayers.begin(), m_pendingPlayers.end(), player);
	if (it == std::end(m_pendingPlayers))
	{
		m_pendingPlayers.emplace_back(player);
		return true;
	}
	return false;
}

void SpawnManager::removePlayerFromQueue(Player* player)
{
	if (m_pendingPlayers.empty())
		return;

	m_pendingPlayers.remove(player);
}

void SpawnManager::onPlayersInPlace()
{
	NS_ASSERT(!m_pendingPlayers.empty());

	this->reset();

	this->prepareItemBoxs();
	this->prepareItems();
	this->processPendingPlayers();
	if(!m_map->isTrainingGround())
		this->fillRobotsIfNeeded();
}

bool SpawnManager::isAllPlayersHere() const
{
	return m_theater->getOnlineCount() == m_map->getPlayerList().size();
}

void SpawnManager::reset()
{
}

SimpleItemBoxList const* SpawnManager::getSimpleItemBoxList(uint32 gridId) const
{
	auto it = m_simpleItemBoxListStore.find(gridId);
	if (it != m_simpleItemBoxListStore.end())
		return &((*it).second);

	return nullptr;
}

ItemBoxItemList const* SpawnManager::getItemBoxItemList(uint32 lootId) const
{
	auto it = m_itemBoxItemListStore.find(lootId);
	if (it != m_itemBoxItemListStore.end())
		return &((*it).second);

	return nullptr;
}

SimpleItemList const* SpawnManager::getSimpleItemList(uint32 gridId) const
{
	auto it = m_simpleItemListStore.find(gridId);
	if (it != m_simpleItemListStore.end())
		return &((*it).second);

	return nullptr;
}

uint32 SpawnManager::getItemCount(uint32 itemId) const
{
	auto it = m_itemCounters.find(itemId);
	if (it != m_itemCounters.end())
		return (*it).second;

	return 0;
}

uint32 SpawnManager::getClassifiedItemCount(uint32 itemClass) const
{
	auto it = m_classifiedItemCounters.find(itemClass);
	if (it != m_classifiedItemCounters.end())
		return (*it).second;

	return 0;
}

void SpawnManager::increaseItemCount(uint32 itemId, int32 count)
{
	m_itemCounters[itemId] += count;
}

void SpawnManager::increaseClassifiedItemCount(uint32 itemClass, int32 count)
{
	m_classifiedItemCounters[itemClass] += count;
}

bool SpawnManager::fillRobotsIfNeeded()
{
	int32 totalRobots = m_map->getMapData()->getPopulationCap() - m_theater->getOnlineCount() - m_map->getRobotCount();
	if (totalRobots <= 0)
		return false;

	if (m_map->getPlayerList().empty())
		return false;

	this->spawnRobots(totalRobots);
	return true;

}

void SpawnManager::configureSimpleRobot(SimpleRobot& simpleRobot)
{
	// 设置难度等级
	RobotDifficulty const& difficulty = this->selectRobotDifficulty();
	simpleRobot.proficiencyLevel = difficulty.proficiencyLevel;
	simpleRobot.natureType = difficulty.natureType;

	// 设置出生模板
	RobotSpawnInfo const& spawnInfo = this->selectRobotSpawnInfo();
	RobotTemplate const* robotTmpl = sObjectMgr->getRobotTemplate(spawnInfo.templateId);
	NS_ASSERT(robotTmpl);
	simpleRobot.templateId = robotTmpl->id;

	// 设置属性阶段
	uint8 stage = robotTmpl->findStageByCombatPower(difficulty.maxCombatPower, 0);
	simpleRobot.stage = stage;
	uint16 realCP = robotTmpl->getStageStats(stage).combatPower;
	NS_ASSERT(realCP <= difficulty.maxCombatPower);

	simpleRobot.level = this->generateRobotLevel(spawnInfo, difficulty.proficiencyLevel);
	if (m_map->isTrainingGround())
	{
		simpleRobot.country = "";
		simpleRobot.name = sObjectMgr->getRobotDefaultName(m_map->getPrimaryLang());
	}
	else
	{
		std::string country = this->generateRobotCountry();
		simpleRobot.country = country;
		simpleRobot.name = this->generateRobotName(country);
	}
}

void SpawnManager::spawnRobots(int32 numOfRobots)
{
	for (int32 i = 0; i < numOfRobots; ++i)
	{
		SimpleRobot simpleRobot;
		this->configureSimpleRobot(simpleRobot);
		Robot* robot = m_map->takeReusableObject<Robot>();
		if (robot)
			robot->reloadData(simpleRobot, m_map);
		else
		{
			robot = new Robot();
			robot->loadData(simpleRobot, m_map);
		}
		m_map->addToMap(robot);
		NS_LOG_DEBUG("world.map.spawn", "Spawn Robot (guid: 0x%08X) templateId: %d proficiency: %d natureType: %d stage: %d combatPower: %d level: %d spawnPoint: %d,%d", robot->getData()->getGuid().getRawValue(), simpleRobot.templateId, simpleRobot.proficiencyLevel, simpleRobot.natureType, simpleRobot.stage, robot->getData()->getCombatPower(), simpleRobot.level, robot->getData()->getSpawnPoint().x, robot->getData()->getSpawnPoint().y);
	}
}

std::string SpawnManager::generateRobotCountry()
{
	auto* regionList = sObjectMgr->getRobotRegionList();
	auto* weightList = sObjectMgr->getRobotRegionWeightList();
	auto ranIt = randomWeightedContainerElement(*regionList, *weightList);
	NS_ASSERT(ranIt != regionList->end());

	return (*ranIt).country;
}

uint8 SpawnManager::generateRobotLevel(RobotSpawnInfo const& spawnInfo, uint8 proficiencyLevel)
{
	uint8 level = 0;
	if (m_map->isTrainingGround())
		return level;

	auto difficultyList = sObjectMgr->getRobotDifficultyList(m_map->getCombatGrade().grade);
	NS_ASSERT(difficultyList && !difficultyList->empty());

	NS_ASSERT(spawnInfo.maxLevel > spawnInfo.minLevel);
	uint8 diffLevel = spawnInfo.maxLevel - spawnInfo.minLevel;
	int32 numDiffic = (int32)difficultyList->size();
	float avgLevels = diffLevel / (float)numDiffic;

	uint8 minProfic = difficultyList->front().proficiencyLevel;
	NS_ASSERT(proficiencyLevel >= minProfic);
	int32 mult = proficiencyLevel - minProfic;
	float sum = spawnInfo.minLevel + mult * avgLevels;
	int32 minSubLevel = (int32)std::ceil(sum);
	int32 maxSubLevel = (int32)std::floor(sum + avgLevels);
	minSubLevel = std::min(minSubLevel, maxSubLevel);
	minSubLevel = std::max((int32)spawnInfo.minLevel, minSubLevel);
	maxSubLevel = std::min((int32)spawnInfo.maxLevel, maxSubLevel);
	level = (uint8)random(minSubLevel, maxSubLevel);

	return level;
}

std::string SpawnManager::generateRobotName(std::string const& country)
{
	std::string name;
	bool noName = rollChance(this->getMap()->getCombatGrade().robotNoNameChance);
	if (noName)
	{
		LangType lang = sObjectMgr->getRobotLangByCountry(country);
		name = sObjectMgr->generatePlayerName(lang);
	}
	else
		name = sObjectMgr->getNextRobotNameByCountry(country);

	return name;
}

RobotDifficulty const& SpawnManager::selectRobotDifficulty()
{
	auto difficultyList = sObjectMgr->getRobotDifficultyList(m_map->getCombatGrade().grade);
	NS_ASSERT(difficultyList);
	auto difficultyWeightList = sObjectMgr->getRobotDifficultyWeightList(m_map->getCombatGrade().grade);
	NS_ASSERT(difficultyWeightList);

	auto ranIt = randomWeightedContainerElement(*difficultyList, *difficultyWeightList);
	NS_ASSERT(ranIt != difficultyList->end());
	return *ranIt;
}

RobotSpawnInfo const& SpawnManager::selectRobotSpawnInfo()
{
	auto spawnList = sObjectMgr->getRobotSpawnList(m_map->getCombatGrade().grade);
	NS_ASSERT(spawnList);
	auto spawnWeightList = sObjectMgr->getRobotSpawnWeightList(m_map->getCombatGrade().grade);
	NS_ASSERT(spawnWeightList);

	auto ranIt = randomWeightedContainerElement(*spawnList, *spawnWeightList);
	NS_ASSERT(ranIt != spawnList->end());
	return *ranIt;
}

void SpawnManager::prepareItemBoxs()
{
	m_simpleItemBoxListStore.clear();
	m_itemBoxItemListStore.clear();
	m_itemCounters.clear();
	m_classifiedItemCounters.clear();

	auto const* spawnList = sObjectMgr->getItemBoxSpawnList(m_map->getMapId());
	auto const* distrList = sObjectMgr->getItemBoxDistributionList(m_map->getMapId());
	NS_ASSERT(!spawnList->empty());

	std::vector<int32> distrIndexes;
	for (int32 i = 0; i < (int32)distrList->size(); ++i)
		distrIndexes.emplace_back(i);
	int32 distrCount = (int32)distrIndexes.size();

	for (auto it = spawnList->begin(); it != spawnList->end(); ++it)
	{
		auto const& spawnInfo = *it;
		
		int32 ranIndex = random(0, (int32)(distrCount - 1));
		int32 distrIndex = distrIndexes[ranIndex];
		distrIndexes.erase(distrIndexes.begin() + ranIndex);
		distrIndexes.push_back(distrIndex);
		--distrCount;
		if (distrCount <= 0)
			distrCount = (int32)distrIndexes.size();

		ObjectDistribution const& distr = (*distrList)[distrIndex];
		ItemBoxTemplate const* tmpl = sObjectMgr->getItemBoxTemplate(spawnInfo.templateId);
		NS_ASSERT(tmpl);
		SimpleItemBox simpleItemBox;
		simpleItemBox.templateId = tmpl->id;
		simpleItemBox.spawnPoint = m_map->randomOpenTileInArea(distr.spawnArea, true);
		simpleItemBox.direction = random((int32)DataItemBox::RIGHT_DOWN, (int32)DataItemBox::LEFT_DOWN);
		simpleItemBox.lootId = spawnInfo.lootId;
		this->addSimpleItemBoxToGrid(simpleItemBox);
		NS_LOG_DEBUG("world.map.spawn", "Prepare ItemBox templateId: %d spawnPoint: %d,%d direction: %d lootId: %d", simpleItemBox.templateId, simpleItemBox.spawnPoint.x, simpleItemBox.spawnPoint.y, simpleItemBox.direction, simpleItemBox.lootId);

		this->rollItemBoxLoot(tmpl, spawnInfo.lootId);
		m_map->setTileClosed(simpleItemBox.spawnPoint);
	}

	NS_LOG_DEBUG("world.map.spawn", "==== Item Statistics ====");
	for (auto pr : m_classifiedItemCounters)
	{
		NS_LOG_DEBUG("world.map.spawn", "ItemClass: %d Count: %d", pr.first, pr.second);
	}
	for (auto pr : m_itemCounters)
	{
		NS_LOG_DEBUG("world.map.spawn", "ItemId: %d Count: %d", pr.first, pr.second);
	}
}

void SpawnManager::addSimpleItemBoxToGrid(SimpleItemBox const& simpleItemBox)
{
	Point position = simpleItemBox.spawnPoint.computePosition(m_map->getMapData()->getMapSize());
	GridCoord coord = computeGridCoordForPosition(position);
	m_simpleItemBoxListStore[coord.getId()].emplace_back(simpleItemBox);
}

void SpawnManager::rollItemBoxLoot(ItemBoxTemplate const* itemBoxTemplate, uint32 lootId)
{
	ItemBoxLootTemplate const* lootTmpl = sObjectMgr->getItemBoxLootTemplate(this->getMap()->getMapId(), lootId);
	if (!lootTmpl)
		return;

	// 根据几率生成物品
	for (auto it = lootTmpl->itemList.begin(); it != lootTmpl->itemList.end(); ++it)
	{
		LootItem const& lootItem = *it;
		if (rollChance(lootItem.chance))
		{
			int32 itemCount = random(lootItem.minCount, lootItem.maxCount);
			ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(lootItem.itemId);
			NS_ASSERT(tmpl);
			if (tmpl->itemClass == ITEM_CLASS_MAGIC_BEAN)
			{
				int32 maxCount = MAGIC_BEAN_DROPPED_MAX_STACK_COUNT;
				int32 nStacks = itemCount / maxCount + ((itemCount % maxCount) ? 1 : 0);
				for (int32 i = 0; i < nStacks; ++i)
				{
					int32 count = std::min(itemCount, maxCount);
					this->createItemBoxItem(lootId, tmpl, count);
					itemCount -= maxCount;
				}
			}
			else if (tmpl->itemClass == ITEM_CLASS_GOLD)
				this->createItemBoxItem(lootId, tmpl, itemCount);
			else
			{
				for (int32 i = 0; i < itemCount; ++i)
					this->createItemBoxItem(lootId, tmpl, 1);
			}
		}
	}
}

void SpawnManager::createItemBoxItem(uint32 lootId, ItemTemplate const* tmpl, int32 count)
{
	ItemBoxItem item;
	item.itemId = tmpl->id;
	item.count = count;
	m_itemBoxItemListStore[lootId].emplace_back(item);
	m_itemCounters[tmpl->id] += count;
	m_classifiedItemCounters[tmpl->itemClass] += count;

	NS_LOG_DEBUG("world.map.spawn", "ItemId: %d Count: %d", tmpl->id, count);
}

void SpawnManager::prepareItems()
{
	m_simpleItemListStore.clear();

	auto const* spawnInfos = sObjectMgr->getItemSpawnInfos(m_map->getMapId());
	if (spawnInfos)
	{
		for (auto it = spawnInfos->begin(); it != spawnInfos->end(); ++it)
		{
			auto const& spawnInfo = (*it).second;

			ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(spawnInfo.itemId);
			NS_ASSERT(tmpl);
			SimpleItem simpleItem;
			simpleItem.templateId = tmpl->id;
			simpleItem.spawnInfoId = spawnInfo.id;
			simpleItem.spawnPoint = m_map->randomOpenTileOnCircle(spawnInfo.spawnPos, spawnInfo.spawnDist, true);

			int32 count = random(spawnInfo.minCount, spawnInfo.maxCount);
			simpleItem.count = count;
			m_itemCounters[tmpl->id] += count;
			m_classifiedItemCounters[tmpl->itemClass] += count;

			this->addSimpleItemToGrid(simpleItem);
			NS_LOG_DEBUG("world.map.spawn", "Prepare Item templateId: %d spawnPoint: %d,%d spawnTime: %d", simpleItem.templateId, simpleItem.spawnPoint.x, simpleItem.spawnPoint.y, spawnInfo.spawnTime);
		}
	}
}

void SpawnManager::addSimpleItemToGrid(SimpleItem const& simpleItem)
{
	Point position = simpleItem.spawnPoint.computePosition(m_map->getMapData()->getMapSize());
	GridCoord coord = computeGridCoordForPosition(position);
	m_simpleItemListStore[coord.getId()].emplace_back(simpleItem);
}

void SpawnManager::processPendingPlayers()
{
	if (m_pendingPlayers.empty())
		return;

	for (auto it = m_pendingPlayers.begin(); it != m_pendingPlayers.end(); ++it)
	{
		Player* player = *it;
		player->spawn(m_map);
		m_map->addPlayerToMap(player);

		player->sendInitialPacketsAfterAddToMap();
	}

	m_pendingPlayers.clear();
}