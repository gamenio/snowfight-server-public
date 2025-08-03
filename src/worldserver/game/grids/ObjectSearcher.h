#ifndef __OBJECT_SEARCHER_H__
#define __OBJECT_SEARCHER_H__

#include "game/entities/updates/UpdateObject.h"
#include "GridDefines.h"


class WorldObject;
class Player;
class LocatorObject;

class PlayerRangeExistsObjectFilter
{
public:
	PlayerRangeExistsObjectFilter(WorldObject* object) :
		m_object(object) { }

	bool operator()(Player* target);

private:
	WorldObject* m_object;
};

class PlayerRangeExistsLocatorFilter
{
public:
	PlayerRangeExistsLocatorFilter(LocatorObject* object) :
		m_object(object) { }

	bool operator()(Player* target);

private:
	LocatorObject* m_object;
};


class PlayerClientExistsObjectFilter
{
public:
	PlayerClientExistsObjectFilter(WorldObject* object) :
		m_object(object) { }

	bool operator()(Player* target);

private:
	WorldObject* m_object;
};

class PlayerClientExistsLocatorFilter
{
public:
	PlayerClientExistsLocatorFilter(LocatorObject* object) :
		m_object(object) { }

	bool operator()(Player* target);

private:
	LocatorObject* m_object;
};

class PlayerRangeContainsOneOfObjectsFilter
{
public:
	PlayerRangeContainsOneOfObjectsFilter(std::vector<WorldObject*> const& objects, std::set<Player*> const& exclusions) :
		m_objects(objects),
		m_exclusions(exclusions) { }

	PlayerRangeContainsOneOfObjectsFilter(std::vector<WorldObject*> const& objects) :
		m_objects(objects) { }

	bool operator()(Player* target);

private:
	std::vector<WorldObject*> m_objects;
	std::set<Player*> m_exclusions;
};


template<typename TARGET, typename CONDITION>
class ObjectSearcher
{
public:
	ObjectSearcher(CONDITION& condition, std::list<TARGET*>& result) :
		m_condition(condition),
		m_result(result) { }

	void visit(GridRefManager<TARGET>& g)
	{
		for (auto it = g.begin(); it != g.end(); ++it)
		{
			TARGET* target = it->getSource();
			if (m_condition(target))
				m_result.push_back(target);
		}
	}

	template<typename T>
	void visit(GridRefManager<T>&) { }

private:
	CONDITION& m_condition;
	std::list<TARGET*>& m_result;
};

class ValuesUpdateAccumulator
{
public:
	ValuesUpdateAccumulator(WorldObject& object, PlayerUpdateMapType& updateMap) :
		m_object(object),
		m_updateMap(updateMap) { }

	void visit(PlayerGridType& g);
	void buildUpdate(Player* player);

	template<typename T>
	void visit(GridRefManager<T>&) { }

private:
	WorldObject& m_object;
	PlayerUpdateMapType& m_updateMap;
};

class LocatorValuesUpdateAccumulator
{
public:
	LocatorValuesUpdateAccumulator(LocatorObject& object, PlayerUpdateMapType& updateMap) :
		m_object(object),
		m_updateMap(updateMap) 
	{
	}

	void visit(PlayerGridType& g);
	void buildUpdate(Player* player);

	template<typename T>
	void visit(GridRefManager<T>&) { }

private:
	LocatorObject& m_object;
	PlayerUpdateMapType& m_updateMap;
};

#endif // __OBJECT_SEARCHER_H__
