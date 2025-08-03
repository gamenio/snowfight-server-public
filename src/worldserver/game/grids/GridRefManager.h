#ifndef __GRID_REFMANAGER_H__
#define __GRID_REFMANAGER_H__

#include "game/dynamic/reference/RefManager.h"

template<class OBJECT>
class GridReference;

template<class OBJECT>
class GridRefManager : public RefManager<GridRefManager<OBJECT>, OBJECT>
{
public:
	typedef LinkedListHead::Iterator< GridReference<OBJECT> > iterator;

	GridReference<OBJECT>* getFirst() { return (GridReference<OBJECT>*)RefManager<GridRefManager<OBJECT>, OBJECT>::getFirst(); }
	GridReference<OBJECT>* getLast() { return (GridReference<OBJECT>*)RefManager<GridRefManager<OBJECT>, OBJECT>::getLast(); }

	iterator begin() { return iterator(getFirst()); }
	iterator end() { return iterator(NULL); }
	iterator rbegin() { return iterator(getLast()); }
	iterator rend() { return iterator(NULL); }
};

#endif // __GRID_REFMANAGER_H__

