#include "ObjectGridLoader.h"

#include "game/behaviors/Player.h"
#include "game/behaviors/Robot.h"
#include "game/behaviors/ItemBox.h"
#include "game/behaviors/Item.h"

template<typename SIMPLE_OBJ, typename OBJECT>
void loadHelper(std::vector<SIMPLE_OBJ> const* simpleObjList, GridRefManager<OBJECT>& g, BattleMap* map, GridCoord const& gridCoord)
{
	for (auto it = simpleObjList->begin(); it != simpleObjList->end(); ++it)
	{
		SIMPLE_OBJ const& simpleObj = *it;

		OBJECT* obj = map->takeReusableObject<OBJECT>();
		if (obj)
			obj->reloadData(simpleObj, map);
		else
		{
			obj = new OBJECT();
			obj->loadData(simpleObj, map);
		}

		obj->addToGrid(g);
		obj->setMap(map);
		obj->addToWorld();

		obj->updateObjectVisibility();
		obj->updateObjectTraceability();
		obj->updateObjectSafety();
	}
}

void ObjectGridLoader::visit(ItemBoxGridType& g)
{
	SimpleItemBoxList const* simpleObjList = m_map->getSpawnManager()->getSimpleItemBoxList(m_gridCoord.getId());
	if (simpleObjList)
		loadHelper(simpleObjList, g, m_map, m_gridCoord);
}

void ObjectGridLoader::visit(ItemGridType & g)
{
	SimpleItemList const* simpleObjList = m_map->getSpawnManager()->getSimpleItemList(m_gridCoord.getId());
	if (simpleObjList)
		loadHelper(simpleObjList, g, m_map, m_gridCoord);
}