#include "CarriedItem.h"

#include "game/behaviors/Unit.h"
#include "game/world/ObjectMgr.h"

CarriedItem::CarriedItem() :
	m_owner(nullptr)
{
    m_type |= TypeMask::TYPEMASK_CARRIED_ITEM;
    m_typeId = TypeID::TYPEID_CARRIED_ITEM;
}

CarriedItem::~CarriedItem()
{
	m_owner = nullptr;
}

void CarriedItem::addToWorld()
{
	if (this->isInWorld())
		return;

	Object::addToWorld();
}

void CarriedItem::removeFromWorld()
{
	if (!this->isInWorld())
		return;

	Object::removeFromWorld();
}

PickupStatus CarriedItem::canBeMergedPartlyWith(ItemTemplate const* itemTemplate) const
{
	if (this->getData()->getItemId() != itemTemplate->id)
		return PICKUP_STATUS_ITEM_CANT_STACK;

	if (itemTemplate->stackable != ITEM_STACK_UNLIMITED && this->getData()->getCount() >= itemTemplate->stackable)
		return PICKUP_STATUS_ITEM_STACK_LIMIT_EXCEEDED;

	return PICKUP_STATUS_OK;
}

void CarriedItem::drop()
{
	TileCoord currCoord(m_owner->getMap()->getMapData()->getMapSize(), m_owner->getData()->getPosition());
	FloorItemPlace itemPlace(m_owner->getMap(), currCoord, FloorItemPlace::LEFT_DOWN);

	SimpleItem simpleItem;
	simpleItem.templateId = this->getData()->getItemId();
	simpleItem.holder = m_owner->getData()->getGuid();
	simpleItem.holderOrigin = m_owner->getData()->getPosition();
	simpleItem.spawnPoint = itemPlace.nextEmptyTile();
	simpleItem.launchCenter = m_owner->getData()->getLaunchCenter();
	simpleItem.dropDuration = random(ITEM_DROP_DELAY_MIN, ITEM_DROP_DELAY_MAX) + ITEM_PARABOLA_DURATION;

	Item* item = m_owner->getMap()->takeReusableObject<Item>();
	if (item)
		item->reloadData(simpleItem, m_owner->getMap());
	else
	{
		item = new Item();
		item->loadData(simpleItem, m_owner->getMap());
	}
	m_owner->getMap()->addToMap(item);
	item->updateObjectVisibility(true);
}

void CarriedItem::buildValuesUpdate(PlayerUpdateMapType& updateMap)
{
	if (Player* player = m_owner->asPlayer())
		this->buildValuesUpdateForPlayerInMap(player, updateMap);
}

bool CarriedItem::addToObjectUpdate()
{
	if (!this->isInWorld())
		return false;

	if (m_owner)
		m_owner->getMap()->addUpdateObject(this);

	return true;
}

void CarriedItem::removeFromObjectUpdate()
{
	if (m_owner)
		m_owner->getMap()->removeUpdateObject(this);
}

bool CarriedItem::loadData(SimpleCarriedItem const& simpleCarriedItem)
{
	NS_ASSERT(m_owner);
	DataCarriedItem* data = new DataCarriedItem();
	data->setGuid(ObjectGuid(GUIDTYPE_CARRIED_ITEM, m_owner->generateCarriedItemSpawnId()));

	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(simpleCarriedItem.itemId);
	NS_ASSERT(tmpl);

	data->setItemId(simpleCarriedItem.itemId);
	data->setCount(simpleCarriedItem.count);
	data->setSlot(simpleCarriedItem.slot);
	data->setLevel(tmpl->level);
	data->setStackable(tmpl->stackable);

	ItemApplicationTemplate const* appTmpl = sObjectMgr->getItemApplicationTemplate(tmpl->appId);
	if (appTmpl)
	{
		data->setCooldownDuration(appTmpl->recoveryTime * 1000);
	}

	this->setData(data);

	return true;
}