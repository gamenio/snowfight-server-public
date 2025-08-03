#ifndef __TILE_AREA_H__
#define __TILE_AREA_H__

#include <algorithm>

#include "TileCoord.h"

struct TileArea
{
	TileArea() {}
	TileArea(TileCoord const& low, TileCoord const& high) :
		lowBound(low),
		highBound(high) {}

	TileCoord lowBound;
	TileCoord highBound;

	inline TileCoord getCenter() const
	{
		return TileCoord(lowBound.x + (highBound.x - lowBound.x) / 2, lowBound.y + (highBound.y - lowBound.y) / 2);
	}

	inline bool containsTileCoord(TileCoord const& coord) const
	{
		return (coord.x >= lowBound.x && coord.y >= lowBound.y && coord.x <= highBound.x && coord.y <= highBound.y);
	}
};


inline TileArea computeTileAreaInViewport(Point const& position, Size const& viewport, Size const& mapSize)
{
	Point center(position);

	// 始终在viewport的中心位置
	float halfWidth = viewport.width * 0.5f;
	float halfHeight = viewport.height * 0.5f;
	center.x = std::max(center.x, halfWidth);
	center.y = std::max(center.y, halfHeight);
	center.x = std::min(center.x, mapSize.width * TILE_WIDTH - halfWidth);
	center.y = std::min(center.y, mapSize.height * TILE_HEIGHT - halfHeight);


	TileCoord topLeft(mapSize, Point(center.x - halfWidth, center.y + halfHeight));
	TileCoord topRight(mapSize, Point(center.x + halfWidth, center.y + halfHeight));
	TileCoord bottomLeft(mapSize, Point(center.x - halfWidth, center.y - halfHeight));
	TileCoord bottomRight(mapSize, Point(center.x + halfWidth, center.y - halfHeight));

	TileCoord low(std::max(topLeft.x, 0), std::max(topRight.y, 0));
	TileCoord high(std::min(bottomRight.x, (int32)(mapSize.width - 1)), std::min(bottomLeft.y, (int32)(mapSize.height - 1)));

	return TileArea(low, high);
}



#endif // __TILE_AREA_H__