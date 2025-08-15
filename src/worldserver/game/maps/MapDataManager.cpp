#include "MapDataManager.h"

#include "Application.h"
#include "logging/Log.h"
#include "SQLiteCpp/SQLiteCpp.h"

MapDataManager::MapDataManager()
{
}


MapDataManager::~MapDataManager()
{
	this->unload();
}

MapDataManager* MapDataManager::instance()
{
	static MapDataManager instance;
	return &instance;
}

bool MapDataManager::load()
{
	// Load map configuration
	std::string dbFile = StringUtil::format("%smap/map.db", sApplication->getRoot().c_str());
	try
	{
		SQLite::Database db(dbFile);
		try
		{
			SQLite::Statement query(db, "SELECT * FROM map_config");
			while (query.executeStep())
			{
				MapConfig config;
				config.id = query.getColumn("id");
				config.populationCap = query.getColumn("population_cap");
				config.battleDuration = query.getColumn("battle_duration");
				m_mapConfigs.emplace_back(config);
			}

		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load map config failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM map_grade");
			std::unordered_map<uint8, double> weightSums;
			while (query.executeStep())
			{
				uint8 combatGrade = query.getColumn("combat_grade");
				MapGradeList& mapGradeList = m_mapGradeListStore[combatGrade];
				MapGrade mapGrade;
				mapGrade.combatGrade = combatGrade;
				mapGrade.mapId = query.getColumn("map_id");
				mapGradeList.emplace_back(mapGrade);

				double weight = query.getColumn("weight").getDouble();
				m_mapGradeWeightListStore[combatGrade].emplace_back(weight);
				weightSums[combatGrade] += weight;
			}

			for (auto it = weightSums.begin(); it != weightSums.end(); ++it)
			{
				double weightSum = (*it).second;
				if (weightSum <= 0.0)
				{
					auto& weights = m_mapGradeWeightListStore[(*it).first];
					weights.assign(weights.size(), 1.0);
				}
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load map grade failed. error: %s", e.what());
			return false;
		}
	}
	catch (std::exception& e)
	{
		NS_LOG_INFO("server.loading", "Failed to load DB file %s. error: %s", dbFile.c_str(), e.what());
		return false;
	}

	// Load map data
	for (auto it = m_mapConfigs.begin(); it != m_mapConfigs.end(); ++it)
	{
		MapConfig& config = *it;
		MapData* data = new MapData(config);

		if (data->loadData())
			m_mapDataSet[config.id] = data;
		else
		{
			delete data;
			return false;
		}

	}

	NS_LOG_INFO("server.loading", "Loaded %zu map data.", m_mapConfigs.size());
	return true;
}

void MapDataManager::unload()
{
	for (auto it = m_mapDataSet.begin(); it != m_mapDataSet.end();)
	{
		MapData* data = it->second;

		it = m_mapDataSet.erase(it);
		delete data;
	}

}

MapData* MapDataManager::findMapData(uint16 mapId) const
{
	auto it = m_mapDataSet.find(mapId);
	if (it != m_mapDataSet.end())
		return (*it).second;

	return nullptr;
}

MapGradeList const* MapDataManager::getMapGradeList(uint8 combatGrade) const
{
	auto it = m_mapGradeListStore.find(combatGrade);
	if (it == m_mapGradeListStore.end())
		return nullptr;

	return &(*it).second;
}

std::vector<double> const* MapDataManager::getMapGradeWeightList(uint8 combatGrade) const
{
	auto it = m_mapGradeWeightListStore.find(combatGrade);
	if (it == m_mapGradeWeightListStore.end())
		return nullptr;

	return &(*it).second;
}