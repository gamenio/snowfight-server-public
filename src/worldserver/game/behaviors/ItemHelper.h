#ifndef __ITEM_HELPER_H__
#define __ITEM_HELPER_H__

#include "Common.h"
#include "game/maps/BattleMap.h"

class FloorItemPlace
{
public:
	enum Direction
	{
		RIGHT_DOWN,
		LEFT_DOWN,
	};

	FloorItemPlace(BattleMap* map, TileCoord const& origin, Direction dir);
	~FloorItemPlace();

	TileCoord nextEmptyTile();

private:
	TileCoord nextTileFromRightDown();
	TileCoord nextTileFromLeftDown();

	BattleMap* m_map;

	TileCoord m_origin;
	Direction m_direction;
	bool m_isClockwise;
	int32 m_currRange;
	int32 m_numOfSteps;
	int32 m_currStep;
	int32 m_stepX;
	int32 m_stepY;
};


#endif // __ITEM_HELPER_H__