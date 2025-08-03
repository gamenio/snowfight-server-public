#include "ObjectAccessor.h"


#include "game/behaviors/Player.h"
#include "game/behaviors/Robot.h"
#include "game/behaviors/Projectile.h"
#include "game/behaviors/ItemBox.h"
#include "game/behaviors/Item.h"

template<typename T>
void ObjectMapHolder<T>::insert(T* o)
{
	boost::unique_lock<boost::shared_mutex> lock(*getMutex());

	getContainer()[o->getData()->getGuid()] = o;
}

template<typename T>
void ObjectMapHolder<T>::remove(T* o)
{
	boost::unique_lock<boost::shared_mutex> lock(*getMutex());

	getContainer().erase(o->getData()->getGuid());
}

template<typename T>
T* ObjectMapHolder<T>::find(ObjectGuid const& guid)
{
	boost::shared_lock<boost::shared_mutex> lock(*getMutex());

	typename ObjectMapType::iterator it = getContainer().find(guid);
	return it != getContainer().end() ? it->second : nullptr;
}

template<typename T>
typename ObjectMapHolder<T>::ObjectMapType& ObjectMapHolder<T>::getContainer()
{
	static ObjectMapType container;
	return container;
}

template<typename T>
boost::shared_mutex* ObjectMapHolder<T>::getMutex()
{
	static boost::shared_mutex mutex;
	return &mutex;
}

WorldObject* ObjectAccessor::getWorldObject(WorldObject const* u, ObjectGuid const& guid)
{
	switch (guid.getType())
	{
	case GUIDTYPE_PLAYER:
		return getPlayer(u, guid);
	case GUIDTYPE_ROBOT:
		return getRobot(u, guid);
	case GUIDTYPE_ITEMBOX:
		return getItemBox(u, guid);
	case GUIDTYPE_ITEM:
		return getItem(u, guid);
	case GUIDTYPE_PROJECTILE:
		return getProjectile(u, guid);
	default:
		return nullptr;
	}
}

Unit* ObjectAccessor::getUnit(WorldObject const* u, ObjectGuid const& guid)
{
	Unit* unit = nullptr;

	if (guid.isPlayer())
		unit = findPlayer(guid);
	else
		unit = getRobot(u, guid);

	return unit;
}

Player* ObjectAccessor::getPlayer(WorldObject const* u, ObjectGuid const& guid)
{
	if (Player* player = PlayerMapHolder::find(guid))
		if (player->getMap() == u->getMap())
			return player;

	return nullptr;
}

Robot* ObjectAccessor::getRobot(WorldObject const* u, ObjectGuid const& guid)
{
	return u->getMap()->getObjectsStore().find<Robot>(guid);
}

Projectile* ObjectAccessor::getProjectile(WorldObject const* u, ObjectGuid const& guid)
{
	return u->getMap()->getObjectsStore().find<Projectile>(guid);
}

ItemBox* ObjectAccessor::getItemBox(WorldObject const* u, ObjectGuid const& guid)
{
	return u->getMap()->getObjectsStore().find<ItemBox>(guid);
}

Item* ObjectAccessor::getItem(WorldObject const* u, ObjectGuid const& guid)
{
	return u->getMap()->getObjectsStore().find<Item>(guid);
}

Player* ObjectAccessor::findPlayer(ObjectGuid const& guid)
{
	return PlayerMapHolder::find(guid);
}

template <>
void ObjectAccessor::addObject<Player>(Player* player)
{
	PlayerMapHolder::insert(player);
}

template <typename T> 
void ObjectAccessor::addObject(T* object)
{
	object->getMap()->getObjectsStore().template insert<T>(object->getData()->getGuid(), object);
}

template <>
void ObjectAccessor::removeObject<Player>(Player* player)
{
	PlayerMapHolder::remove(player);
}

template <typename T>
void ObjectAccessor::removeObject(T* object)
{
	object->getMap()->getObjectsStore().template remove<T>(object->getData()->getGuid());
}

template class ObjectMapHolder<Player>;

template void ObjectAccessor::addObject<Player>(Player*);
template void ObjectAccessor::removeObject<Player>(Player*);
template void ObjectAccessor::addObject<Robot>(Robot*);
template void ObjectAccessor::removeObject<Robot>(Robot*);
template void ObjectAccessor::addObject<Projectile>(Projectile*);
template void ObjectAccessor::removeObject<Projectile>(Projectile*);
template void ObjectAccessor::addObject<Item>(Item*);
template void ObjectAccessor::removeObject<Item>(Item*);
template void ObjectAccessor::addObject<ItemBox>(ItemBox*);
template void ObjectAccessor::removeObject<ItemBox>(ItemBox*);
