#include "Item.h"

#include "utilities/TimeUtil.h"
#include "game/world/ObjectMgr.h"
#include "game/world/ObjectAccessor.h"
#include "game/utils/MathTools.h"
#include "ObjectShapes.h"
#include "game/behaviors/CarriedItem.h"
#include "game/behaviors/Robot.h"
#include "game/behaviors/Player.h"
#include "logging/Log.h"


Item::Item() :
	m_spawnInfoId(0),
	m_itemState(ITEM_STATE_NONE),
	m_activationTimer(0),
	m_respawnTimer(0)
{
    m_type |= TypeMask::TYPEMASK_ITEM;
    m_typeId = TypeID::TYPEID_ITEM;
}

Item::~Item()
{
}

void Item::update(NSTime diff)
{
	WorldObject::update(diff);

	if (!this->isInWorld())
		return;

	switch (m_itemState)
	{
	case ITEM_STATE_ACTIVATING:
	{
		int32 elapsed = (int32)(getUptimeMillis() - m_activationTimer);
		elapsed = std::min(elapsed, this->getData()->getDropDuration());
		this->getData()->setDropElapsed(elapsed);

		if (elapsed >= this->getData()->getDropDuration())
			this->activate();
		break;
	}
	case ITEM_STATE_INACTIVE:
	{
		ItemSpawnInfo const* spawnInfo = sObjectMgr->getItemSpawnInfo(this->getMap()->getMapId(), m_spawnInfoId);
		NS_ASSERT(spawnInfo);

		int32 elapsed = (int32)(getUptimeMillis() - m_respawnTimer);
		if (elapsed >= spawnInfo->spawnTime * 1000)
			this->respawn();
		break;
	}
	default:
		break;
	}
}

void Item::addToWorld()
{
	if (this->isInWorld())
		return;

	WorldObject::addToWorld();

	ObjectAccessor::addObject(this);

	TileCoord currCoord(m_map->getMapData()->getMapSize(), this->getData()->getPosition());
	m_map->addTileFlag(currCoord, TILE_FLAG_ITEM_PLACED);
}

void Item::removeFromWorld()
{
	if (!this->isInWorld())
		return;

	TileCoord currCoord(m_map->getMapData()->getMapSize(), this->getData()->getPosition());
	m_map->clearTileFlag(currCoord, TILE_FLAG_ITEM_PLACED);

	this->invisibleForNearbyPlayers();
	ObjectAccessor::removeObject(this);

	WorldObject::removeFromWorld();
}

void Item::cleanupBeforeDelete()
{
	if (this->isInWorld())
		this->removeFromWorld();

	this->removeAllCollidedObjects();
	this->removeAllPickers();
	this->removeAllCollectors();
	m_followerRefManager.clearReferences();
	m_interestedRefManager.clearReferences();

	WorldObject::cleanupBeforeDelete();
}

void Item::addCollector(Robot* collector)
{
	auto it = m_collectors.find(collector);
	if (it == m_collectors.end())
		m_collectors.emplace(collector);
}

void Item::removeCollector(Robot* collector)
{
	auto it = m_collectors.find(collector);
	if (it != m_collectors.end())
		m_collectors.erase(it);
}

void Item::removeAllCollectors()
{
	while (!m_collectors.empty())
	{
		auto it = m_collectors.begin();
		if (!(*it)->removeCollectTarget())
		{
			NS_LOG_ERROR("behaviors.item", "WORLD: Item has an collector that isn't collecting it!");
			m_collectors.erase(it);
		}
	}
}

bool Item::isCollector(Robot* robot) const
{
	auto it = m_collectors.find(robot);
	return it != m_collectors.end();
}

void Item::addPicker(Unit* picker)
{
	auto it = m_pickers.find(picker);
	if (it == m_pickers.end())
		m_pickers.emplace(picker);
}

void Item::removePicker(Unit* picker)
{
	auto it = m_pickers.find(picker);
	if (it != m_pickers.end())
		m_pickers.erase(it);
}

void Item::removeAllPickers()
{
	while (!m_pickers.empty())
	{
		auto it = m_pickers.begin();
		if (!(*it)->pickupStop())
		{
			NS_LOG_ERROR("behaviors.item", "WORLD: Item has an picker that isn't picking it up!");
			m_pickers.erase(it);
		}
	}
}

bool Item::isPicker(Unit* unit) const
{
	auto it = m_pickers.find(unit);
	return it != m_pickers.end();
}

void Item::receiveBy(Unit* receiver)
{
	if (!this->inactivate())
		m_map->addObjectToRemoveList(this);
}

bool Item::canBeMergedWith(CarriedItem* carriedItem)
{
	if (!this->isActive())
		return false;

	if (this->getData()->getItemId() != carriedItem->getData()->getItemId())
		return false;

	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(this->getData()->getItemId());
	NS_ASSERT(tmpl);
	if (tmpl->stackable != ITEM_STACK_UNLIMITED && carriedItem->getData()->getCount() >= tmpl->stackable)
		return false;

	return true;
}

bool Item::canDetect(Unit* target) const
{
	if (!this->isActive() || !target->isInWorld() || !target->isVisible() || !target->isAlive())
		return false;

	// The item cannot detect GM players
	if (target->asPlayer() && target->asPlayer()->getData()->isGM())
		return false;

	return true;
}

void Item::testCollision(Unit* target)
{
	if (this->isCollideWith(target))
		this->addCollidedObject(target);
	else
		this->removeCollidedObject(target);
}

bool Item::loadData(SimpleItem const& simpleItem, BattleMap* map)
{
	DataItem* data = new DataItem();
	data->setGuid(ObjectGuid(GUIDTYPE_ITEM, map->generateItemSpawnId()));
	this->setData(data);
	
	return this->reloadData(simpleItem, map);
}

bool Item::reloadData(SimpleItem const& simpleItem, BattleMap* map)
{
	DataItem* data = this->getData();
	if (!data)
		return false;

	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(simpleItem.templateId);
	NS_ASSERT(tmpl);

	ItemSpawnInfo const* spawnInfo = sObjectMgr->getItemSpawnInfo(map->getMapId(), simpleItem.spawnInfoId);
	m_spawnInfoId = simpleItem.spawnInfoId;

	data->setObjectRadiusInMap(ITEM_OBJECT_RADIUS_IN_MAP);

	data->setItemId(tmpl->id);
	if(!simpleItem.spawnPoint.isInvalid())
		data->setPosition(simpleItem.spawnPoint.computePosition(map->getMapData()->getMapSize()));

	data->setCount(simpleItem.count);
	data->setHolder(simpleItem.holder);
	data->setHolderOrigin(simpleItem.holderOrigin);
	data->setLaunchCenter(simpleItem.launchCenter);
	data->setLaunchRadiusInMap(simpleItem.launchRadiusInMap);
	data->setDropDuration(simpleItem.dropDuration);
	data->setDropElapsed(0);

	if (!spawnInfo)
		this->setItemState(ITEM_STATE_ACTIVATING);
	else
	{
		if (spawnInfo->spawnTime > 0)
			this->setItemState(ITEM_STATE_INACTIVE);
		else
			this->setItemState(ITEM_STATE_ACTIVE);
	}

	return true;
}

bool Item::isCollideWith(Unit* target)
{
	MapData* mapData = this->getMap()->getMapData();
	Point selfMapPos = mapData->openGLToMapPos(this->getData()->getPosition());
	Point pickerMapPos = mapData->openGLToMapPos(target->getData()->getPosition());
	float radius = target->getData()->getObjectRadiusInMap() + this->getData()->getObjectRadiusInMap();
	if (MathTools::isPointInsideCircle(selfMapPos, radius, pickerMapPos))
	{
		return true;
	}

	return false;
}

void Item::addCollidedObject(Unit* object)
{
	auto it = m_collidedObjects.find(object->getData()->getGuid());
	if (it == m_collidedObjects.end())
	{
		ItemCollisionReference* ref = new ItemCollisionReference(object, this);
		auto it = m_collidedObjects.emplace(object->getData()->getGuid(), ref);
		NS_ASSERT(it.second);

		object->enterCollision(this);
	}
	else
		object->stayCollision(this);
}

void Item::removeCollidedObject(Unit* object)
{
	auto it = m_collidedObjects.find(object->getData()->getGuid());
	if (it != m_collidedObjects.end())
	{
		ItemCollisionReference* ref = (*it).second;
		ref->unlink();
		it = m_collidedObjects.erase(it);
		delete ref;

		object->leaveCollision(this);
	}
}

void Item::removeAllCollidedObjects()
{
	for (auto it = m_collidedObjects.begin(); it != m_collidedObjects.end();)
	{
		ItemCollisionReference* ref = (*it).second;
		Unit* object = ref->getTarget();
		ref->unlink();
		it = m_collidedObjects.erase(it);
		delete ref;

		if(object)
			object->leaveCollision(this);
	}
}

void Item::respawn()
{
	ItemSpawnInfo const* spawnInfo = sObjectMgr->getItemSpawnInfo(this->getMap()->getMapId(), m_spawnInfoId);
	NS_ASSERT(spawnInfo);

	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(spawnInfo->itemId);
	NS_ASSERT(tmpl);

	SpawnManager* spawnMgr = m_map->getSpawnManager();

	// Random count
	int32 count = random(spawnInfo->minCount, spawnInfo->maxCount);
	this->getData()->setCount(count);
	spawnMgr->increaseItemCount(tmpl->id, count);
	spawnMgr->increaseClassifiedItemCount(tmpl->itemClass, count);

	this->setItemState(ITEM_STATE_ACTIVE);
	this->updateObjectVisibility();
}

void Item::resetRespawnTimer()
{
	m_respawnTimer = getUptimeMillis();
}

bool Item::inactivate()
{
	ItemSpawnInfo const* spawnInfo = sObjectMgr->getItemSpawnInfo(this->getMap()->getMapId(), m_spawnInfoId);
	if (spawnInfo && spawnInfo->spawnTime > 0)
	{
		// If the random position is within the safe zone, set it to inactive state
		TileCoord coord = this->getMap()->randomOpenTileOnCircle(spawnInfo->spawnPos, spawnInfo->spawnDist, true);
		if (this->getMap()->isWithinSafeZone(coord))
		{
			// Force update visibility state
			this->setItemState(ITEM_STATE_INACTIVE);
			this->updateObjectVisibility(true);

			Point newPosition = coord.computePosition(this->getMap()->getMapData()->getMapSize());
			this->getMap()->itemRelocation(this, newPosition);

			return true;
		}
	}

	return false;
}

void Item::activate()
{
	this->setItemState(ITEM_STATE_ACTIVE);
	this->updateObjectVisibility();
}

void Item::setItemState(ItemState state)
{
	m_itemState = state;
	switch (state)
	{
	case ITEM_STATE_ACTIVATING:
		this->resetActivationTimer();
		break;
	case ITEM_STATE_ACTIVE:
		m_activationTimer = 0;
		m_respawnTimer = 0;
		this->setVisible(true);
		break;
	case ITEM_STATE_INACTIVE:
		this->removeAllCollidedObjects();
		this->removeAllPickers();
		this->removeAllCollectors();
		m_observerRefManager.clearReferences();
		m_followerRefManager.clearReferences();
		m_interestedRefManager.clearReferences();

		this->setVisible(false);
		this->resetRespawnTimer();
		break;
	default:
		break;
	}
}

void Item::resetActivationTimer()
{
	m_activationTimer = getUptimeMillis();
}
