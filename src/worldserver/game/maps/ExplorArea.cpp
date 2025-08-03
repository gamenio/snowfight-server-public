#include "ExplorArea.h"

ExplorArea::ExplorArea() :
	x(0),
	y(0)
{
}

ExplorArea::ExplorArea(int32 xx, int32 yy) :
	x(xx),
	y(yy)
{
}

ExplorArea::ExplorArea(Point const& pos, int32 areaSize, Point areaOffset)
{
	this->x = (int32)((pos.x - areaOffset.x) / areaSize);
	this->y = (int32)((pos.y - areaOffset.y) / areaSize);
}

const ExplorArea ExplorArea::INVALID(-1, -1);


Point ExplorArea::computePosition(int32 areaSize, Point areaOffset) const
{
	float x = this->x * areaSize + areaSize / 2 + areaOffset.x;
	float y = this->y * areaSize + areaSize / 2 + areaOffset.y;

	return Point(x, y);
}

bool ExplorArea::containsPoint(Point const& point, int32 areaSize, Point areaOffset) const
{
	bool bRet = false;

	float minX = this->x * areaSize + areaOffset.x;
	float minY = this->y * areaSize + areaOffset.y;
	float maxX = this->x * areaSize + areaSize + areaOffset.x;
	float maxY = this->y * areaSize + areaSize + areaOffset.y;
	if (point.x >= minX && point.x <= maxX
		&& point.y >= minY && point.y <= maxY)
	{
		bRet = true;
	}

	return bRet;
}