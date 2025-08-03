#ifndef __INTERESTED_REFMANAGER_H__
#define __INTERESTED_REFMANAGER_H__

#include "game/dynamic/reference/RefManager.h"
#include "InterestedReference.h"

class Item;
class WishManager;

class InterestedRefManager: public RefManager<Item, WishManager>
{
public:
	InterestedReference* getFirst() { return ((InterestedReference*)RefManager<Item, WishManager>::getFirst()); }
};

#endif // __INTERESTED_REFMANAGER_H__
