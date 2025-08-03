#include "ItemHelper.h"

FloorItemPlace::FloorItemPlace(BattleMap* map, TileCoord const& origin, Direction dir) :
	m_map(map),
	m_origin(origin),
	m_direction(dir),
	m_isClockwise(true),
	m_currRange(0),
	m_numOfSteps(0),
	m_currStep(0),
	m_stepX(0),
	m_stepY(0)
{

}

FloorItemPlace::~FloorItemPlace()
{
}

TileCoord FloorItemPlace::nextEmptyTile()
{
	TileCoord coord;

	MapData* mapData = m_map->getMapData();
	int32 nTiles = (int32)(mapData->getMapSize().width * mapData->getMapSize().height);
	for (int32 i = 0; i < nTiles; ++i)
	{
		if (m_currStep >= m_numOfSteps - 1)
		{
			++m_currRange;
			m_numOfSteps = (m_currRange * 8);

			m_isClockwise = true;
			m_currStep = 0;
			m_stepX = 0;
			m_stepY = 0;
		}
		else
			++m_currStep;

		switch (m_direction)
		{
		case RIGHT_DOWN:
			coord = this->nextTileFromRightDown();
			break;
		case LEFT_DOWN:
			coord = this->nextTileFromLeftDown();
			break;
		}

		if (mapData->isValidTileCoord(coord) 
			&& !mapData->isWall(coord)
			&& !mapData->isConcealable(coord)
			&& !m_map->hasTileFlag(coord, TILE_FLAG_ITEM_PLACED) && !m_map->hasTileFlag(coord, TILE_FLAG_CLOSED))
		{
			break;
		}
	}


	return coord;
}

TileCoord FloorItemPlace::nextTileFromRightDown()
{
	TileCoord coord;

	int32 startX = m_origin.x + m_currRange;
	int32 startY = m_origin.y;
	int32 side = m_currRange * 2 + 1;
	if (m_isClockwise)
	{
		coord.x = startX - m_stepX;
		coord.y = startY + m_stepY;
		m_isClockwise = false;
	}
	else
	{
		if (m_stepX >= side - 1)
			--m_stepY;
		else
		{
			if (m_stepY < m_currRange)
				++m_stepY;
			else
			{
				if (m_stepX <= side - 1)
					++m_stepX;
			}
		}

		coord.x = startX - m_stepX;
		coord.y = startY - m_stepY;
		m_isClockwise = true;
	}

	return coord;
}

TileCoord FloorItemPlace::nextTileFromLeftDown()
{
	TileCoord coord;

	int32 startX = m_origin.x;
	int32 startY = m_origin.y + m_currRange;
	int32 side = m_currRange * 2 + 1;
	if (m_isClockwise)
	{
		coord.x = startX - m_stepX;
		coord.y = startY - m_stepY;
		m_isClockwise = false;
	}
	else
	{
		if (m_stepY >= side - 1)
			--m_stepX;
		else
		{
			if (m_stepX < m_currRange)
				++m_stepX;
			else
			{
				if (m_stepY <= side - 1)
					++m_stepY;
			}
		}

		coord.x = startX + m_stepX;
		coord.y = startY - m_stepY;
		m_isClockwise = true;
	}

	return coord;
}