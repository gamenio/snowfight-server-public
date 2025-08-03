#ifndef __OBJECT_GRID_LOADER_H__
#define __OBJECT_GRID_LOADER_H__

#include "Common.h"
#include "game/maps/BattleMap.h"
#include "game/behaviors/Robot.h"
#include "game/behaviors/Item.h"
#include "game/behaviors/Projectile.h"
#include "GridDefines.h"

class ObjectGridLoader
{
public:
	ObjectGridLoader(BattleMap* map, GridCoord const& gridCoord) :
		m_map(map),
		m_gridCoord(gridCoord) { }
	~ObjectGridLoader() {}

	void visit(ItemBoxGridType& g);
	void visit(ItemGridType& g);
	template<typename T> void visit(GridRefManager<T>&) {}

private:
	BattleMap* m_map;
	GridCoord m_gridCoord;
};


class ObjectGridCleaner
{
public:
	ObjectGridCleaner(BattleMap* map) :
		m_map(map) { }
	~ObjectGridCleaner() {}

	void visit(PlayerGridType& g)
	{
		NS_ASSERT(g.isEmpty(), "Cleanup player should not be handled by ObjectGridCleaner.");
	}

	template<typename T>
	void visit(GridRefManager<T>& g)
	{
		while (!g.isEmpty())
		{
			T* obj = g.getFirst()->getSource();
			obj->cleanupBeforeDelete();
			m_map->removeFromMap(obj, false);
		}
	}

private:
	BattleMap* m_map;
};

class ObjectGridUnloader
{
public:
	ObjectGridUnloader(BattleMap* map) :
		m_map(map) { }
	~ObjectGridUnloader() {}

	void visit(PlayerGridType& g)
	{
		NS_ASSERT(g.isEmpty(), "Unload player should not be handled by ObjectGridUnloader.");
	}

	template<typename T>
	void visit(GridRefManager<T>& g)
	{
		while (!g.isEmpty())
		{
			T* obj = g.getFirst()->getSource();
			obj->cleanupBeforeDelete();
			delete obj;
		}
	}

private:
	BattleMap* m_map;
};

#endif // __OBJECT_GRID_LOADER_H__
