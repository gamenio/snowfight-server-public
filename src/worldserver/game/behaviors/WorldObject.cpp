#include "WorldObject.h"

#include "game/grids/ObjectSearcher.h"
#include "game/grids/GridNotifiers.h"
#include "Player.h"

WorldObject::WorldObject():
	m_map(nullptr),
	m_notifyflags(NOTIFY_NONE),
	m_isVisible(true),
	m_locator(nullptr)
{
	m_type |= TypeMask::TYPEMASK_WORLDOBJECT;
	m_typeId = TypeID::TYPEID_WORLDOBJECT;
}


WorldObject::~WorldObject()
{
	if (m_locator)
	{
		m_locator->setOwner(nullptr);
		delete m_locator;
		m_locator = nullptr;
	}
	m_map = nullptr;
}

void WorldObject::setMap(BattleMap* map)
{
	if (m_map != map)
	{
		DataWorldObject* data = this->getData();
		NS_ASSERT(data, "Need to set data for WorldObject before setMap()");
		if (map)
			data->setMapData(map->getMapData());
		else
			data->setMapData(nullptr);
		m_map = map;
	}
}

void WorldObject::setLocator(LocatorObject* locator)
{
	if (m_locator != locator)
	{
		if (m_locator)
		{
			delete m_locator;
			m_locator = nullptr;
		}
		locator->setOwner(this);
		m_locator = locator;
	}
}

void WorldObject::removeFromWorld()
{
	if (!this->isInWorld())
		return;

	this->resetAllNotifies();

	Object::removeFromWorld();
}

void WorldObject::cleanupBeforeDelete()
{
	if (this->isInWorld())
		this->removeFromWorld();

	m_observerRefManager.clearReferences();
}

void WorldObject::destroyForNearbyPlayers()
{
	PlayerClientExistsObjectFilter filter(this);
	std::list<Player*> result;
	ObjectSearcher<Player, PlayerClientExistsObjectFilter> searcher(filter, result);

	this->visitNearbyObjectsInMaxVisibleRange(searcher);

	for (auto it = result.begin(); it != result.end(); ++it)
	{
		Player* player = *it;
		this->sendDestroyToPlayer(player);
		player->removeFromClient(this);
	}

#if NS_DEBUG
	{
		// 确保所有玩家都从客户端对象列表中移除了当前对象
		PlayerClientExistsObjectFilter filter(this);
		std::list<Player*> result;
		ObjectSearcher<Player, PlayerClientExistsObjectFilter> searcher(filter, result);
		this->visitAllObjects(searcher);

		NS_ASSERT(result.empty());
	}
#endif

}

void WorldObject::invisibleForNearbyPlayers()
{
	PlayerClientExistsObjectFilter filter(this);
	std::list<Player*> result;
	ObjectSearcher<Player, PlayerClientExistsObjectFilter> searcher(filter, result);

	this->visitNearbyObjectsInMaxVisibleRange(searcher);

	for (auto it = result.begin(); it != result.end(); ++it)
	{
		Player* player = *it;
		this->sendOutOfRangeUpdateToPlayer(player);
		player->removeFromClient(this);
	}
}

void WorldObject::buildValuesUpdate(PlayerUpdateMapType& updateMap)
{
	// 收集所有可以看到当前对象的玩家以及与玩家相关的更新数据
	ValuesUpdateAccumulator accumulator(*this, updateMap);
	this->visitNearbyObjectsInMaxVisibleRange(accumulator);
}

void WorldObject::updateObjectVisibility(bool forced)
{
	if (!forced)
		this->addToNotify(NOTIFY_VISIBILITY_CHANGED);
	else
	{
		VisibleChangesNotifier notifier(*this);
		this->visitNearbyObjectsInMaxVisibleRange(notifier);

		this->clearNotify(NOTIFY_VISIBILITY_CHANGED);
	}
}

void WorldObject::updateObjectSafety()
{
	this->addToNotify(NOTIFY_SAFETY_CHANGED);
}

void WorldObject::updateObjectTraceability(bool forced)
{
	if (!this->getLocator())
		return;

	if (!forced)
		this->addToNotify(NOTIFY_TRACEABILITY_CHANGED);
	else
	{
		TrackingChangesNotifier notifier(*this);
		this->visitAllObjects(notifier);

		this->clearNotify(NOTIFY_TRACEABILITY_CHANGED);
	}
}

Rect WorldObject::getBoundingBox() const
{
	return Rect(this->getData()->getPosition().x - this->getData()->getObjectSize().width * this->getData()->getAnchorPoint().x,
		this->getData()->getPosition().y - this->getData()->getObjectSize().height * this->getData()->getAnchorPoint().y,
		this->getData()->getObjectSize().width,
		this->getData()->getObjectSize().height);
}

bool WorldObject::isWithinDist(Point const& pos, float dist) const
{
	bool ret = this->getData()->getPosition().getDistance(pos) <= dist;
	return  ret;
}

bool WorldObject::isWithinDist(WorldObject const* obj, float dist) const
{
	return isWithinDist(obj->getData()->getPosition(), dist);
}

bool WorldObject::addToObjectUpdate()
{
	if (!this->isInWorld())
		return false;

	getMap()->addUpdateObject(this);
	return true;
}

void WorldObject::removeFromObjectUpdate()
{
	getMap()->removeUpdateObject(this);
}

void AttackableObject::cleanupBeforeDelete()
{
	if (this->isInWorld())
		this->removeFromWorld();

	this->removeAllWatchers();
	this->removeAllAttackers();
	m_projCollisionRefManager.clearReferences();

	WorldObject::cleanupBeforeDelete();
}

void AttackableObject::removeAllAttackers()
{
	while (!m_attackers.empty())
	{
		auto it = m_attackers.begin();
		if (!(*it)->attackStop())
		{
			NS_LOG_ERROR("behaviors.worldobject", "WORLD: AttackableObject has an attacker that isn't attacking it!");
			m_attackers.erase(it);
		}
	}
}