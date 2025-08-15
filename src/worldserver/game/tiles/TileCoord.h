#ifndef __TILE_COORD_H__
#define __TILE_COORD_H__

#include <math.h>

#include "Common.h"
#include "TileDefines.h"
#include "game/entities/Point.h"
#include "game/entities/Size.h"


class MapData;

// The tile coordinates of the unit on the map.
struct TileCoord
{
	TileCoord() :x(0), y(0) 
	{
	}

	TileCoord(int32 xx, int32 yy) :
		x(xx), y(yy) 
	{
	}

	TileCoord(Size const& mapSize, Point const& position)
	{
		float ix = position.x - mapSize.width * TILE_WIDTH_HALF;
		float iy = mapSize.height * TILE_HEIGHT - position.y;
		float tx = floor((ix / TILE_WIDTH_HALF + iy / TILE_HEIGHT_HALF) / 2);
		float ty = floor((iy / TILE_HEIGHT_HALF - ix / TILE_WIDTH_HALF) / 2);
		this->x = (int32)std::max(0.f, std::min(mapSize.width - 1, tx));
		this->y = (int32)std::max(0.f, std::min(mapSize.height - 1, ty));
	}

	bool operator==(TileCoord const& right) const
	{
		return (this->x == right.x && this->y == right.y);
	}

	bool operator!=(TileCoord const& right) const
	{
		return !(*this == right);
	}

	TileCoord operator-(TileCoord const& right) const
	{
		return TileCoord(this->x - right.x, this->y - right.y);
	}

	TileCoord operator+(TileCoord const& right) const
	{
		return TileCoord(this->x + right.x, this->y + right.y);
	}


	void setTileCoord(int32 x, int32 y)
	{
		this->x = x;
		this->y = y;
	}

	Point computePosition(Size const& mapSize) const
	{
		float x = mapSize.width * TILE_WIDTH_HALF + (this->x - this->y) * TILE_WIDTH_HALF;
		float y = mapSize.height * TILE_HEIGHT - (this->x + this->y) * TILE_HEIGHT_HALF - TILE_HEIGHT_HALF;

		return Point(x, y);
	}

	float getDistance(TileCoord const& coord) const
	{
		int32 dx = coord.x - x;
		int32 dy = coord.y - y;

		return (float)std::sqrt(dx * dx + dy * dy);
	}

	bool isInvalid() const
	{  
		return x == TileCoord::INVALID.x && y == TileCoord::INVALID.y;
	}

	bool isZero() const
	{
		return x == 0 && y == 0;
	}

	static const TileCoord ZERO;
	static const TileCoord INVALID;

	int32 x;
	int32 y;
};



#endif // __TILE_COORD_H__