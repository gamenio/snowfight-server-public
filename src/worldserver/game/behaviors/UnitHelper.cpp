#include "UnitHelper.h"

#include "game/maps/BattleMap.h"

bool isObstructed(BattleMap* map, TileCoord const& coord)
{
	MapData* mapData = map->getMapData();
	if (mapData->isCollidable(coord) || map->isTileClosed(coord))
		return true;

	return false;
}


Point UnitHelper::computeLandingPosition(DataUnit const* launcher, float direction)
{
	return computeLandingPosition(launcher->getPosition(), launcher->getAttackRange(), direction);
}

Point UnitHelper::computeLandingPosition(Point const& launcherPos, float attackRange, float direction)
{
	Point landingPos;
	Point startPos = launcherPos;
	float dist = attackRange;
	float dx = cos(direction) * dist;
	float dy = sin(direction) * dist;
	landingPos.setPoint(startPos.x + dx, startPos.y + dy);

	return landingPos;
}

Point UnitHelper::computeLaunchPosition(DataUnit const* launcher, Point const& targetPos)
{
	return computeLaunchPosition(launcher->getMapData(), launcher->getPosition(), launcher->getLaunchCenter(), launcher->getLaunchRadiusInMap(), targetPos);
}

Point UnitHelper::computeLaunchPosition(MapData const* mapData, Point const& launcherPos, Point const& launchCenter, float launchRadiusInMap, Point const& targetPos)
{
	Point launcherMapPos = mapData->openGLToMapPos(launcherPos);
	Point targetMapPos = mapData->openGLToMapPos(targetPos);
	Point point = MathTools::findPointAlongLine(launcherMapPos, targetMapPos, launchRadiusInMap);
	point = mapData->mapToOpenGLPos(point);
	Point result;
	result.x = point.x;
	result.y = point.y + launchCenter.y;
	return result;
}

// Bresenham's line algorithm
//https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
//http://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm
bool UnitHelper::checkLineOfSight(BattleMap* map, TileCoord const& fromCoord, TileCoord const& toCoord)
{
	int32 x1 = fromCoord.x;
	int32 y1 = fromCoord.y;
	int32 x2 = toCoord.x;
	int32 y2 = toCoord.y;

	const bool steep = (abs(y2 - y1) > abs(x2 - x1));
	if (steep)
	{
		std::swap(x1, y1);
		std::swap(x2, y2);
	}

	if (x1 > x2)
	{
		std::swap(x1, x2);
		std::swap(y1, y2);
	}

	const int32 dx = x2 - x1;
	const int32 dy = abs(y2 - y1);

	float error = dx / 2.0f;
	const int32 ystep = (y1 < y2) ? 1 : -1;
	int32 y = y1;

	const int32 maxX = x2;

	TileCoord current;
	bool gap = false; // Fill gaps when Y changes to prevent the line of sight from crossing the edge of the tile
	TileCoord side1;
	TileCoord side2;
	for (int32 x = x1; x <= maxX; x++)
	{
		if (steep)
		{
			current.setTileCoord(y, x);
			if (gap)
			{
				side1.setTileCoord(y, x - 1);
				side2.setTileCoord(y - ystep, x);
			}
		}
		else
		{
			current.setTileCoord(x, y);
			if (gap)
			{
				side1.setTileCoord(x - 1, y);
				side2.setTileCoord(x, y - ystep);
			}
		}

		if (gap)
		{
			if (isObstructed(map, side1) || isObstructed(map, side2))
				return false;
		}

		if (x > x1 && x < maxX)
		{
			if (isObstructed(map, current))
				return false;
		}

		gap = false;
		error -= dy;
		if (error < 0)
		{
			y += ystep;
			error += dx;

			gap = true;
		}
	}

	return true;
}