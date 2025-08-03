#ifndef __UNIT_SPAWN_POINT_GENERATOR_H__
#define __UNIT_SPAWN_POINT_GENERATOR_H__

#include "Common.h"
#include "game/tiles/TileCoord.h"
#include "game/world/ObjectMgr.h"

class BattleMap;

class UnitSpawnPointGenerator
{
public:
	UnitSpawnPointGenerator(BattleMap const* map);
	~UnitSpawnPointGenerator();

	void initialize();

	void reset(bool isShuffle);
	TileCoord nextPosition();

private:
	BattleMap const* m_map;
	int32 m_nextDistrIndex;
	ObjectDistributionList m_distrList;
};

#endif // __UNIT_SPAWN_POINT_GENERATOR_H__