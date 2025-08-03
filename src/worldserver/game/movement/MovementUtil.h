#ifndef __MOVEMENT_UTIL_H__
#define __MOVEMENT_UTIL_H__

#include "Common.h"
#include "game/behaviors/Unit.h"

class MovementUtil
{
public:
	static bool calcTileCoordToBypassTarget(Unit* agent, Unit* target, TileCoord const& goalCoord, TileCoord& result);
};


#endif // __MOVEMENT_UTIL_H__