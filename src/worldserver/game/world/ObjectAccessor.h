#ifndef __OBJECT_ACCESSOR_H__
#define __OBJECT_ACCESSOR_H__

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

#include "Common.h"
#include "game/entities/DataTypes.h"
#include "game/entities/ObjectGuid.h"

template<typename T>
class ObjectMapHolder
{
public:
	typedef std::unordered_map<ObjectGuid, T*> ObjectMapType;

	static void insert(T* o);
	static void remove(T* o);
	static T* find(ObjectGuid const& guid);

	static ObjectMapType& getContainer();
	static boost::shared_mutex* getMutex();
};

class WorldObject;
class Unit;
class Robot;
class Player;
class Projectile;
class ItemBox;
class Item;

namespace ObjectAccessor
{
	typedef ObjectMapHolder<Player> PlayerMapHolder;

	WorldObject* getWorldObject(WorldObject const* u, ObjectGuid const& guid);
	Unit* getUnit(WorldObject const* u, ObjectGuid const& guid);
	Player* getPlayer(WorldObject const* u, ObjectGuid const& guid);
	Robot* getRobot(WorldObject const* u, ObjectGuid const& guid);
	Projectile* getProjectile(WorldObject const* u, ObjectGuid const& guid);
	ItemBox* getItemBox(WorldObject const* u, ObjectGuid const& guid);
	Item* getItem(WorldObject const* u, ObjectGuid const& guid);

	// 在整个世界中查找玩家。对返回对象的操作不是线程安全的
	Player* findPlayer(ObjectGuid const& guid);

	template <typename T> void addObject(T* object);
	template <typename T> void removeObject(T* object);
};

#endif // __OBJECT_ACCESSOR_H__