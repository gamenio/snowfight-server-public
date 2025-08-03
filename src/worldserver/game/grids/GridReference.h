#ifndef __GRID_REFERENCE_H__
#define __GRID_REFERENCE_H__

#include "game/dynamic/reference/Reference.h"

template<class OBJECT>
class GridRefManager;

template<class OBJECT>
class GridReference : public Reference<GridRefManager<OBJECT>, OBJECT>
{
public:
	~GridReference() { this->unlink(); }
	GridReference* next() { return (GridReference*)Reference<GridRefManager<OBJECT>, OBJECT>::next(); }

protected:
	void buildLink() override
	{
		this->getTarget()->insertFirst(this);
		this->getTarget()->incSize();
	}
	void destroyLink() override
	{
		this->getTarget()->decSize();
	}

};

#endif // __GRID_REFERENCE_H__

