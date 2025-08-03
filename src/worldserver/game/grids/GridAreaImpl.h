#ifndef __GRID_AREA_IMPL_H__
#define __GRID_AREA_IMPL_H__

#include "game/maps/BattleMap.h"
#include "GridArea.h"


template<typename VISITOR>
inline void GridArea::visitGrids(BattleMap& map, VISITOR& visitor, bool isCreateGrid) const
{
	TypeContainerVisitor<VISITOR, WorldTypeGridContainer> worldObjectVisitor(visitor);
	for (uint32 x = lowBound.x; x <= highBound.x; ++x)
	{
		for (uint32 y = lowBound.y; y <= highBound.y; ++y)
		{
			GridCoord coord(x, y);
			map.visit(coord, worldObjectVisitor, isCreateGrid);
		}
	}
}


#endif //__GRID_AREA_IMPL_H__