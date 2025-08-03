#include "ItemCollisionReference.h"

#include "game/behaviors/Unit.h"

ItemCollisionReference::ItemCollisionReference(Unit* target, Item* source)
{
	this->link(target, source);
}

ItemCollisionReference::~ItemCollisionReference()
{
	this->unlink();
}

void ItemCollisionReference::buildLink()
{
	this->getTarget()->getItemCollisionRefManager()->insertFirst(this);
	this->getTarget()->getItemCollisionRefManager()->incSize();
}

void ItemCollisionReference::destroyLink()
{
	this->getTarget()->getItemCollisionRefManager()->decSize();
}