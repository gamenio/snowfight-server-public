#ifndef __TYPE_CONTAINER_FUNCTIONS_H__
#define __TYPE_CONTAINER_FUNCTIONS_H__


template<typename H> struct ContainerGrid;
template<typename H, typename T> struct ContainerUnorderedMap;
template<typename H> struct ContainerUnorderedSet;

namespace TypeContainerFunctions {
	// ContainerUnorderedMap helpers
	// Insert helpers
	template<typename OBJECT, typename KEY>
	bool insert(ContainerUnorderedMap<OBJECT, KEY>& elements, KEY const& key, OBJECT* obj)
	{
		auto i = elements.element.find(key);
		if (i == elements.element.end())
		{
			elements.element[key] = obj;
			return true;
		}
		else
		{
			NS_ASSERT(i->second == obj, "Object with certain key already in but objects are different!");
			return false;
		}
	}

	template<typename OBJECT, typename KEY>
	bool insert(ContainerUnorderedMap<TypeNull, KEY>& /*elements*/, KEY const& /*key*/, OBJECT* /*obj*/)
	{
		return false;
	}

	template<typename OBJECT, typename KEY, typename T>
	bool insert(ContainerUnorderedMap<T, KEY>& /*elements*/, KEY const& /*key*/, OBJECT* /*obj*/)
	{
		return false;
	}

	template<typename OBJECT, typename KEY, typename H, typename T>
	bool insert(ContainerUnorderedMap<TypeList<H, T>, KEY>& elements, KEY const& key, OBJECT* obj)
	{
		bool ret = insert(elements.elements, key, obj);
		return ret ? ret : insert(elements.tailElements, key, obj);
	}

	// Find helpers
	template<typename OBJECT, typename KEY>
	OBJECT* find(ContainerUnorderedMap<OBJECT, KEY> const& elements, KEY const& key, OBJECT* /*obj*/)
	{
		auto i = elements.element.find(key);
		if (i == elements.element.end())
			return nullptr;
		else
			return i->second;
	}

	template<typename OBJECT, typename KEY>
	OBJECT* find(ContainerUnorderedMap<TypeNull, KEY> const& /*elements*/, KEY const& /*key*/, OBJECT* /*obj*/)
	{
		return nullptr;
	}

	template<typename OBJECT, typename KEY, typename T>
	OBJECT* find(ContainerUnorderedMap<T, KEY> const& /*elements*/, KEY const& /*key*/, OBJECT* /*obj*/)
	{
		return nullptr;
	}

	template<typename OBJECT, typename KEY, typename H, typename T>
	OBJECT* find(ContainerUnorderedMap<TypeList<H, T>, KEY> const& elements, KEY const& key, OBJECT* /*obj*/)
	{
		OBJECT* ret = find(elements.elements, key, (OBJECT*)nullptr);
		return ret ? ret : find(elements.tailElements, key, (OBJECT*)nullptr);
	}

	// Count helpers
	template<typename OBJECT, typename KEY>
	size_t count(ContainerUnorderedMap<OBJECT, KEY> const& elements, OBJECT* /*obj*/)
	{
		return elements.element.size();
	}

	template<typename OBJECT, typename KEY>
	size_t count(ContainerUnorderedMap<TypeNull, KEY> const& /*elements*/, OBJECT* /*obj*/)
	{
		return 0;
	}

	template<typename OBJECT, typename KEY, typename T>
	size_t count(ContainerUnorderedMap<T, KEY> const& /*elements*/, OBJECT* /*obj*/)
	{
		return 0;
	}

	template<typename OBJECT, typename KEY, typename T>
	size_t count(ContainerUnorderedMap<TypeList<OBJECT, T>, KEY> const& elements, OBJECT* /*obj*/)
	{
		return count(elements.elements, (OBJECT*)nullptr);
	}

	template<typename OBJECT, typename KEY, typename H, typename T>
	size_t count(ContainerUnorderedMap<TypeList<H, T>, KEY> const& elements, OBJECT* /*obj*/)
	{
		return count(elements.tailElements, (OBJECT*)nullptr);
	}

	// Remove helpers
	template<typename OBJECT, typename KEY>
	bool remove(ContainerUnorderedMap<OBJECT, KEY>& elements, KEY const& key, OBJECT* /*obj*/)
	{
		elements.element.erase(key);
		return true;
	}

	template<typename OBJECT, typename KEY>
	bool remove(ContainerUnorderedMap<TypeNull, KEY>& /*elements*/, KEY const& /*key*/, OBJECT* /*obj*/)
	{
		return false;
	}

	template<typename OBJECT, typename KEY, typename T>
	bool remove(ContainerUnorderedMap<T, KEY>& /*elements*/, KEY const& /*key*/, OBJECT* /*obj*/)
	{
		return false;
	}

	template<typename OBJECT, typename KEY, typename H, typename T>
	bool remove(ContainerUnorderedMap<TypeList<H, T>, KEY>& elements, KEY const& key, OBJECT* /*obj*/)
	{
		bool ret = remove(elements.elements, key, (OBJECT*)nullptr);
		return ret ? ret : remove(elements.tailElements, key, (OBJECT*)nullptr);
	}

	// GetElement helpers
	template<typename OBJECT, typename KEY>
	std::unordered_map<KEY, OBJECT*>& getElement(ContainerUnorderedMap<OBJECT, KEY>& elements, OBJECT* /*obj*/)
	{
		return elements.element;
	}

	template<typename OBJECT, typename KEY, typename T>
	std::unordered_map<KEY, OBJECT*>& getElement(ContainerUnorderedMap<TypeList<OBJECT, T>, KEY>& elements, OBJECT* /*obj*/)
	{
		return getElement(elements.elements, (OBJECT*)nullptr);
	}

	template<typename OBJECT, typename KEY, typename H, typename T>
	std::unordered_map<KEY, OBJECT*>& getElement(ContainerUnorderedMap<TypeList<H, T>, KEY>& elements, OBJECT* /*obj*/)
	{
		return getElement(elements.tailElements, (OBJECT*)nullptr);
	}

	// ContainerMapList helpers
	// Insert helpers
	template<typename OBJECT>
	bool insert(OBJECT*  obj, ContainerGrid<OBJECT>& elements)
	{
		obj->addToGrid(elements.element);
		return true;
	}

	template<typename OBJECT, typename H>
	bool insert(OBJECT*, ContainerGrid<H>&)
	{
		return false;
	}


	template<typename OBJECT, typename H, typename T>
	void insert(OBJECT* obj, ContainerGrid<TypeList<H, T>>& elements)
	{
		bool ret = insert(obj, elements.elements);
		if (!ret)
			insert(obj, elements.tailElements);
	}

	// Remove helpers
	template<typename OBJECT>
	bool remove(OBJECT* obj, ContainerGrid<OBJECT>& elements)
	{
		obj->removeFromGrid();
		return true;
	}

	template<typename OBJECT, typename H>
	bool remove(OBJECT*, ContainerGrid<H>&)
	{
		return false;
	}


	template<typename OBJECT, typename H, typename T>
	void remove(OBJECT* obj, ContainerGrid<TypeList<H, T>>& elements)
	{
		bool ret = remove(obj, elements.elements);
		if (!ret)
			remove(obj, elements.tailElements);
	}

	// ContainerUnorderedSet helpers
	// Insert helpers
	template<typename OBJECT>
	bool insert(ContainerUnorderedSet<OBJECT>& elements, OBJECT* obj)
	{
		elements.element.insert(obj);
		return true;
	}

	template<typename OBJECT, typename H>
	bool insert(ContainerUnorderedSet<H>&, OBJECT*)
	{
		return false;
	}

	template<typename OBJECT, typename H, typename T>
	bool insert(ContainerUnorderedSet<TypeList<H, T>>& elements, OBJECT* obj)
	{
		bool ret = insert(elements.elements, obj);
		return ret ? ret : insert(elements.tailElements, obj);
	}

	// Remove helpers
	template<typename OBJECT>
	bool remove(ContainerUnorderedSet<OBJECT>& elements, OBJECT* obj)
	{
		auto it = elements.element.find(obj);
		if (it != elements.element.end())
		{
			elements.element.erase(it);
			return true;
		}
			
		return false;
	}

	template<typename OBJECT, typename H>
	bool remove(ContainerUnorderedSet<H>&, OBJECT* obj)
	{
		return false;
	}


	template<typename OBJECT, typename H, typename T>
	bool remove(ContainerUnorderedSet<TypeList<H, T>>& elements, OBJECT* obj)
	{
		bool ret = remove(elements.elements, obj);
		return ret ? ret : remove(elements.tailElements, obj);
	}

	// GetElement helpers
	template<typename OBJECT>
	std::unordered_set<OBJECT*>& getElement(ContainerUnorderedSet<OBJECT>& elements, OBJECT* /*obj*/)
	{
		return elements.element;
	}

	template<typename OBJECT, typename T>
	std::unordered_set<OBJECT*>& getElement(ContainerUnorderedSet<TypeList<OBJECT, T>>& elements, OBJECT* obj)
	{
		return getElement(elements.elements, obj);
	}

	template<typename OBJECT, typename H, typename T>
	std::unordered_set<OBJECT*>& getElement(ContainerUnorderedSet<TypeList<H, T>>& elements, OBJECT* obj)
	{
		return getElement(elements.tailElements, obj);
	}

} //namespace TypeContainerFunctions


#endif //__TYPE_CONTAINER_FUNCTIONS_H__
