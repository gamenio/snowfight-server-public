#include "LocatorObject.h"

#include "game/behaviors/WorldObject.h"
#include "game/behaviors/Player.h"
#include "game/grids/ObjectSearcher.h"

LocatorObject::LocatorObject() :
	m_owner(nullptr)
{
    m_type |= TypeMask::TYPEMASK_LOCATOR_OBJECT;
    m_typeId = TypeID::TYPEID_LOCATOR_OBJECT;
}

LocatorObject::~LocatorObject()
{
	m_owner = nullptr;
}

void LocatorObject::removeFromWorld()
{
	if (!this->isInWorld())
		return;

	this->destroyForNearbyPlayers();

	Object::removeFromWorld();
}

void LocatorObject::destroyForNearbyPlayers()
{
	if (!m_owner)
		return;

	PlayerClientExistsLocatorFilter filter(this);
	std::list<Player*> result;
	ObjectSearcher<Player, PlayerClientExistsLocatorFilter> searcher(filter, result);
	m_owner->visitAllObjects(searcher);

	for (auto it = result.begin(); it != result.end(); ++it)
	{
		Player* player = *it;
		this->sendDestroyToPlayer(player);
		player->removeTrackingFromClient(m_owner);
	}
}

void LocatorObject::invisibleForNearbyPlayers()
{
	if (!m_owner)
		return;

	PlayerClientExistsLocatorFilter filter(this);
	std::list<Player*> result;
	ObjectSearcher<Player, PlayerClientExistsLocatorFilter> searcher(filter, result);
	m_owner->visitAllObjects(searcher);

	for (auto it = result.begin(); it != result.end(); ++it)
	{
		Player* player = *it;
		this->sendOutOfRangeUpdateToPlayer(player);
		player->removeTrackingFromClient(m_owner);
	}
}

void LocatorObject::buildValuesUpdate(PlayerUpdateMapType& updateMap)
{
	if (!m_owner)
		return;

	// 收集所有能够追踪当前定位器所有者的玩家以及与玩家相关的更新数据
	LocatorValuesUpdateAccumulator accumulator(*this, updateMap);
	m_owner->visitAllObjects(accumulator);
}

bool LocatorObject::addToObjectUpdate()
{
	if (!this->isInWorld())
		return false;

	if(m_owner)
		m_owner->getMap()->addUpdateObject(this);

	return true;
}

void LocatorObject::removeFromObjectUpdate()
{
	if (m_owner)
		m_owner->getMap()->removeUpdateObject(this);
}