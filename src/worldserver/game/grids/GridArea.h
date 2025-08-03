#ifndef __GRID_AREA_H__
#define __GRID_AREA_H__

#include <algorithm>

#include "GridDefines.h"
#include "game/tiles/TileDefines.h"
#include "game/tiles/TileCoord.h"
#include "game/entities/Size.h"

class BattleMap;


struct GridArea
{
	GridArea() {}
	GridArea(const GridCoord& low, const GridCoord& high) :
		lowBound(low),
		highBound(high) {}

	GridCoord lowBound;
	GridCoord highBound;

	template<typename VISITOR>
	void visitGrids(BattleMap& map, VISITOR& visitor, bool isCreateGrid = false) const;
};


inline GridArea computeGridAreaInCircle(Point const& center, float radius)
{
	GridCoord lowCoord = computeGridCoordForXY(center.x - radius, center.y - radius);
	GridCoord highCoord = computeGridCoordForXY(center.x + radius, center.y + radius);
	return GridArea(lowCoord, highCoord);
}

inline GridArea computeGridAreaInRect(Point const& center, Size const& size, Size const& mapSize)
{
	Point origin(center);

	float halfWidth = size.width * 0.5f;
	float halfHeight = size.height * 0.5f;

	GridCoord lowCoord = computeGridCoordForXY(origin.x - halfWidth, origin.y - halfHeight);
	GridCoord highCoord = computeGridCoordForXY(origin.x + halfWidth, origin.y + halfHeight);
	return GridArea(lowCoord, highCoord);
}

inline GridArea computeGridAreaInCircle(TileCoord const& center, int32 radius, Size const& mapSize)
{
	TileCoord leftTop;
	TileCoord leftBottom;
	TileCoord rightTop;
	TileCoord rightBottom;

	leftTop.x = center.x - radius;
	leftTop.y = center.y - radius;
	leftBottom.x = center.x - radius;
	leftBottom.y = center.y + radius;
	rightTop.x = center.x + radius;
	rightTop.y = center.y - radius;
	rightBottom.x = center.x + radius;
	rightBottom.y = center.y + radius;

	Point lt = leftTop.computePosition(mapSize);
	Point lb = leftBottom.computePosition(mapSize);
	Point rt = rightTop.computePosition(mapSize);
	Point rb = rightBottom.computePosition(mapSize);

	float minX = lb.x - TILE_WIDTH_HALF;
	float minY = rb.y - TILE_HEIGHT_HALF;
	float maxX = rt.x + TILE_WIDTH_HALF;
	float maxY = lt.y + TILE_HEIGHT_HALF;

	GridCoord lowCoord = computeGridCoordForXY(minX, minY);
	GridCoord highCoord = computeGridCoordForXY(maxX, maxY);

	return GridArea(lowCoord, highCoord);
}


#endif //__GRID_AREA_H__