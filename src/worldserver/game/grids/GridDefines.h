#ifndef __GRID_DEFINES_H__
#define __GRID_DEFINES_H__

#include "Common.h"
#include "game/dynamic/TypeList.h"
#include "game/entities/Point.h"
#include "game/entities/DataTypes.h"
#include "game/entities/ObjectGuid.h"
#include "GridReference.h"

// The number of rows (columns) of the grid in the map, Grid's total area should be able to be enough to cover the size of the map.
#define MAX_NUMBER_OF_GRIDS					23
#define GRID_SIZE							256
#define GRID_SIZE_HALF						(GRID_SIZE / 2)


class Unit;
class Player;
class Robot;
class ItemBox;
class Item;
class Projectile;

template<typename T> class Grid;
template<typename T> class TypeGridContainer;
template<typename T> class TypeUnorderedSetContainer;
template<typename T, typename H> class TypeUnorderedMapContainer;
template<typename T, typename H> class TypeContainerVisitor;

typedef TYPELIST_5(Player, Robot, ItemBox, Item, Projectile) AllWorldObjectTypes;
typedef TYPELIST_4(Robot, ItemBox, Item, Projectile) AllMapStoredObjectTypes;

typedef Grid<AllWorldObjectTypes> GridType;

typedef GridRefManager<Player> PlayerGridType;
typedef GridRefManager<Robot> RobotGridType;
typedef GridRefManager<ItemBox> ItemBoxGridType;
typedef GridRefManager<Item> ItemGridType;
typedef GridRefManager<Projectile> ProjectileGridType;

typedef TypeGridContainer<AllWorldObjectTypes> WorldTypeGridContainer;
typedef TypeUnorderedSetContainer<AllMapStoredObjectTypes> MapStoredObjectSetContainer;
typedef TypeUnorderedMapContainer<AllMapStoredObjectTypes, ObjectGuid> MapStoredObjectMapContainer;

template<uint32 LIMIT>
struct CoordPair
{
	CoordPair(uint32 xx = 0, uint32 yy = 0):
		x(xx),
		y(yy)
	{

	}

	uint32 getId() const
	{
		return y * LIMIT + x;
	}

	uint32 x;
	uint32 y;
};

template<uint32 LIMIT>
bool operator==(const CoordPair<LIMIT>& lhs, const CoordPair<LIMIT>& rhs)
{
	return (lhs.x == rhs.x && lhs.y == rhs.y);
}

template<uint32 LIMIT>
bool operator!=(const CoordPair<LIMIT>& lhs, const CoordPair<LIMIT>& rhs)
{
	return !(lhs == rhs);
}

typedef CoordPair<MAX_NUMBER_OF_GRIDS> GridCoord;


inline GridCoord computeGridCoordForXY(float x, float y)
{
	uint32 xx = std::min(static_cast<uint32>(std::max(x, 0.0f) / GRID_SIZE), static_cast<uint32>(MAX_NUMBER_OF_GRIDS - 1));
	uint32 yy = std::min(static_cast<uint32>(std::max(y, 0.0f) / GRID_SIZE), static_cast<uint32>(MAX_NUMBER_OF_GRIDS - 1));
	return GridCoord(xx, yy);
}

inline GridCoord computeGridCoordForPosition(Point const& position)
{
	return computeGridCoordForXY(position.x, position.y);
}



#endif // __GRID_DEFINES_H__
