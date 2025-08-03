#ifndef __ITEM_COLLISION_REFMANAGER_H__
#define __ITEM_COLLISION_REFMANAGER_H__

#include "game/dynamic/reference/RefManager.h"
#include "ItemCollisionReference.h"

class ItemCollisionRefManager : public RefManager<Unit, Item>
{
public:
	typedef LinkedListHead::Iterator<ItemCollisionReference> iterator;

	ItemCollisionReference* getFirst() { return (ItemCollisionReference*)RefManager::getFirst(); }
	ItemCollisionReference* getLast() { return (ItemCollisionReference*)RefManager::getLast(); }

	iterator begin() { return iterator(getFirst()); }
	iterator end() { return iterator(NULL); }
	iterator rbegin() { return iterator(getLast()); }
	iterator rend() { return iterator(NULL); }
};

#endif // __ITEM_COLLISION_REFMANAGER_H__

