#ifndef __MAP_DATA_MANAGER_H__
#define __MAP_DATA_MANAGER_H__

#include "MapData.h"

typedef std::unordered_map<uint16 /* MapId */, MapData*> MapDataContainer;
typedef std::vector<MapConfig> MapConfigContainer;

typedef std::vector<MapGrade> MapGradeList;
typedef std::unordered_map<uint8 /* CombatGrade */, MapGradeList> MapGradeListContainer;
typedef std::unordered_map<uint8 /* CombatGrade */, std::vector<double>> MapGradeWeightListContainer;

class MapDataManager
{
public:
	static MapDataManager* instance();

	bool load();
	void unload();

	MapData* findMapData(uint16 mapId) const;
	MapDataContainer const& getMapDataSet() const { return m_mapDataSet; }

	MapGradeList const* getMapGradeList(uint8 combatGrade) const;
	std::vector<double> const* getMapGradeWeightList(uint8 combatGrade) const;

private:
	MapDataManager();
	~MapDataManager();

	MapConfigContainer m_mapConfigs;
	MapDataContainer m_mapDataSet;

	MapGradeListContainer m_mapGradeListStore;
	MapGradeWeightListContainer m_mapGradeWeightListStore;
};

#define sMapDataManager MapDataManager::instance()

#endif // __MAP_DATA_MANAGER_H__

