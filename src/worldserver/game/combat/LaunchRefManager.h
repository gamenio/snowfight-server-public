#ifndef __LAUNCH_REFMANAGER_H__
#define __LAUNCH_REFMANAGER_H__

#include "game/dynamic/reference/RefManager.h"

class LaunchReference;

class LaunchRefManager : public RefManager<LaunchRefManager, Unit>
{
public:
	typedef LinkedListHead::Iterator<LaunchReference> iterator;

	LaunchReference* getFirst() { return (LaunchReference*)RefManager::getFirst(); }
	LaunchReference* getLast() { return (LaunchReference*)RefManager::getLast(); }

	iterator begin() { return iterator(getFirst()); }
	iterator end() { return iterator(NULL); }
	iterator rbegin() { return iterator(getLast()); }
	iterator rend() { return iterator(NULL); }
};

#endif // __LAUNCH_REFMANAGER_H__

