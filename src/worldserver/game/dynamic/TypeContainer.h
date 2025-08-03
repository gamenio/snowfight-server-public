#ifndef __TYPE_CONTAINER_H__
#define __TYPE_CONTAINER_H__

#include "TypeList.h"
#include "TypeContainerFunctions.h"
#include "game/grids/GridRefManager.h"

// ContainerGrid

template<typename OBJECT>
struct ContainerGrid
{
	GridRefManager<OBJECT> element;
};

template<>
struct ContainerGrid<TypeNull>
{
};

template<typename H, typename T>
struct ContainerGrid<TypeList<H, T>>
{
	ContainerGrid<H> elements;
	ContainerGrid<T> tailElements;
};

template<typename OBJECT_TYPES>
class TypeGridContainer
{
public:
	template<typename OBJECT>
	void insert(OBJECT* obj)
	{
		TypeContainerFunctions::insert(obj, m_elements);
	}

	template<typename OBJECT>
	void remove(OBJECT* obj)
	{
		TypeContainerFunctions::remove(obj, m_elements);
	}

	ContainerGrid<OBJECT_TYPES>& getElements() { return m_elements; }
	const ContainerGrid<OBJECT_TYPES>& getElements() const{ return m_elements; }

private:
	ContainerGrid<OBJECT_TYPES> m_elements;
};

// ContainerUnorderedMap

template<typename OBJECT, typename KEY>
struct ContainerUnorderedMap
{
	std::unordered_map<KEY, OBJECT*> element;
};

template<typename KEY>
struct ContainerUnorderedMap<TypeNull, KEY>
{
};

template<typename H, typename T, typename KEY>
struct ContainerUnorderedMap<TypeList<H, T>, KEY>
{
	ContainerUnorderedMap<H, KEY> elements;
	ContainerUnorderedMap<T, KEY> tailElements;
};


template<typename OBJECT_TYPES, typename KEY>
class TypeUnorderedMapContainer
{
public:
	template<typename OBJECT>
	bool insert(KEY const& key, OBJECT* obj)
	{
		return TypeContainerFunctions::insert(m_elements, key, obj);
	}

	template<typename OBJECT>
	bool remove(KEY const& key)
	{
		return TypeContainerFunctions::remove(m_elements, key, (OBJECT*)NULL);
	}

	template<typename OBJECT>
	OBJECT* find(KEY const& key)
	{
		return TypeContainerFunctions::find(m_elements, key, (OBJECT*)NULL);
	}

	template<typename OBJECT>
	size_t count() const
	{
		return TypeContainerFunctions::count(m_elements, (OBJECT*)NULL);
	}

	template<typename OBJECT>
	std::unordered_map<KEY, OBJECT*>& getElement()
	{
		return TypeContainerFunctions::getElement(m_elements, (OBJECT*)NULL);
	}

	template<typename OBJECT>
	std::unordered_map<KEY, OBJECT*> const& getElement() const
	{
		return TypeContainerFunctions::getElement(m_elements, (OBJECT*)NULL);
	}

	ContainerUnorderedMap<OBJECT_TYPES, KEY>& getElements() { return m_elements; }
	ContainerUnorderedMap<OBJECT_TYPES, KEY> const& getElements() const { return m_elements; }

private:
	ContainerUnorderedMap<OBJECT_TYPES, KEY> m_elements;
};


// ContainerUnorderedSet

template<typename OBJECT>
struct ContainerUnorderedSet
{
	std::unordered_set<OBJECT*> element;
};

template<>
struct ContainerUnorderedSet<TypeNull>
{
};

template<typename H, typename T>
struct ContainerUnorderedSet<TypeList<H, T>>
{
	ContainerUnorderedSet<H> elements;
	ContainerUnorderedSet<T> tailElements;
};


template<typename OBJECT_TYPES>
class TypeUnorderedSetContainer
{
public:
	template<typename OBJECT>
	bool insert(OBJECT* obj)
	{
		return TypeContainerFunctions::insert(m_elements, obj);
	}

	template<typename OBJECT>
	bool remove(OBJECT* obj)
	{
		return TypeContainerFunctions::remove(m_elements, obj);
	}

	template<typename OBJECT>
	std::unordered_set<OBJECT*>& getElement()
	{
		return TypeContainerFunctions::getElement(m_elements, (OBJECT*)NULL);
	}

	template<typename OBJECT>
	std::unordered_set<OBJECT*> const& getElement() const
	{
		return TypeContainerFunctions::getElement(m_elements, (OBJECT*)NULL);
	}

	ContainerUnorderedSet<OBJECT_TYPES>& getElements() { return m_elements; }
	ContainerUnorderedSet<OBJECT_TYPES> const& getElements() const { return m_elements; }

private:
	ContainerUnorderedSet<OBJECT_TYPES> m_elements;
};

#endif // __TYPE_CONTAINER_H__
