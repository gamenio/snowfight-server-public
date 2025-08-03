#ifndef __GRID_H__
#define __GRID_H__

#include "Common.h"
#include "game/dynamic/TypeContainer.h"
#include "game/dynamic/TypeContainerVisitor.h"
#include "game/tiles/TileCoord.h"
#include "GridDefines.h"



template<typename WORLD_OBJECT_TYPES>
class Grid
{
public:
	explicit Grid(GridCoord const& coord) :
		m_gridCoord(coord),
		m_isGridObjectDataLoaded(false)
	{ 
	}

	~Grid() 
	{
	}

	template<typename OBJECT>
	void addWorldObject(OBJECT* obj)
	{
		m_objects.insert(obj);
	}

	template<typename OBJECT>
	void removeWorldObject(OBJECT* obj)
	{
		m_objects.remove(obj);
	}

	template<typename VISITOR>
	void visit(TypeContainerVisitor<VISITOR, TypeGridContainer<WORLD_OBJECT_TYPES>>& visitor)
	{
		visitor.visit(m_objects);
	}
	
	void setGridObjectDataLoaded(bool b) { m_isGridObjectDataLoaded = b;}
	bool isGridObjectDataLoaded() const { return m_isGridObjectDataLoaded; }

	GridCoord const& getGridCoord() const { return m_gridCoord; }

private:
	GridCoord m_gridCoord;
	bool m_isGridObjectDataLoaded;
	TypeGridContainer<WORLD_OBJECT_TYPES> m_objects;
};


#endif //__GRID_H__
