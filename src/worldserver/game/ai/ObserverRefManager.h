#ifndef __OBSERVER_REFMANAGER_H__
#define __OBSERVER_REFMANAGER_H__

#include "game/dynamic/reference/RefManager.h"
#include "ObserverReference.h"

class Unit;
class TargetSelector;

class ObserverRefManager : public RefManager<WorldObject, TargetSelector>
{
public:
	ObserverReference* getFirst() { return ((ObserverReference*)RefManager<WorldObject, TargetSelector>::getFirst()); }
};

#endif // __OBSERVER_REFMANAGER_H__
