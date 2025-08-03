#include "UnitSpawnPointGenerator.h"

#include "utilities/Random.h"
#include "logging/Log.h"
#include "BattleMap.h"

UnitSpawnPointGenerator::UnitSpawnPointGenerator(BattleMap const* map) :
	m_map(map),
	m_nextDistrIndex(0)
{
}

UnitSpawnPointGenerator::~UnitSpawnPointGenerator()
{
	m_map = nullptr;
}

void UnitSpawnPointGenerator::initialize()
{
	m_distrList = *sObjectMgr->getUnitDistributionList(m_map->getMapId());
}

void UnitSpawnPointGenerator::reset(bool isShuffle)
{
	if(isShuffle)
		std::random_shuffle(m_distrList.begin(), m_distrList.end());
	m_nextDistrIndex = 0;
}

TileCoord UnitSpawnPointGenerator::nextPosition()
{
	auto const& distr = m_distrList[m_nextDistrIndex];
	TileCoord ranCoord;
	ranCoord.x = random(distr.spawnArea.lowBound.x, distr.spawnArea.highBound.x);
	ranCoord.y = random(distr.spawnArea.lowBound.y, distr.spawnArea.highBound.y);

	m_map->findNearestOpenTile(ranCoord, ranCoord, true);

	++m_nextDistrIndex;
	if (m_nextDistrIndex >= m_distrList.size())
		m_nextDistrIndex = 0;

	//NS_LOG_DEBUG("misc", "spwanArea: %d,%d-%d,%d hit (tile): %d,%d",
	//	distr.spawnArea.lowBound.x, distr.spawnArea.lowBound.y,
	//	distr.spawnArea.highBound.x, distr.spawnArea.highBound.y,
	//	ranCoord.x, ranCoord.y);

	return ranCoord;
}