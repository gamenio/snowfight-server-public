#ifndef __EXPLOR_AREA_H__
#define __EXPLOR_AREA_H__

#include "Common.h"
#include "game/tiles/TileCoord.h"

class ExplorArea
{
public:
	ExplorArea();
	ExplorArea(int32 xx, int32 yy);
	ExplorArea(Point const& pos, int32 areaSize, Point areaOffset);

	static const ExplorArea INVALID;

	int32 x;
	int32 y;
	TileCoord originInTile;

	Point computePosition(int32 areaSize, Point areaOffset) const;
	bool containsPoint(Point const& point, int32 areaSize, Point areaOffset) const;
};

inline bool operator==(ExplorArea const& lhs, ExplorArea const& rhs) {
	return (lhs.x == rhs.x && lhs.y == rhs.y);
}


inline bool operator!=(ExplorArea const& lhs, ExplorArea const& rhs) {
	return !(lhs == rhs);
}


inline ExplorArea operator-(ExplorArea const& lhs, ExplorArea const& rhs) {
	return ExplorArea(lhs.x - rhs.x, lhs.y - rhs.y);
}

inline ExplorArea operator+(ExplorArea const& lhs, ExplorArea const& rhs) {
	return ExplorArea(lhs.x + rhs.x, lhs.y + rhs.y);
}

#endif // __EXPLOR_AREA_H__