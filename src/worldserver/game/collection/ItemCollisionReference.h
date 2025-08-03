#ifndef __ITEM_COLLISION_REFERENCE_H__
#define __ITEM_COLLISION_REFERENCE_H__

#include "game/dynamic/reference/Reference.h"

class Unit;
class Item;

class ItemCollisionReference : public Reference<Unit, Item>
{
public:
	ItemCollisionReference(Unit* target, Item* source);
	~ItemCollisionReference();

	ItemCollisionReference* next() { return (ItemCollisionReference*)Reference<Unit, Item>::next(); }

protected:
	void buildLink() override;
	void destroyLink() override;
};

#endif // __ITEM_COLLISION_REFERENCE_H__

