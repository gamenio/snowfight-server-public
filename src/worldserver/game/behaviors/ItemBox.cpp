#include "ItemBox.h"

#include "logging/Log.h"
#include "game/world/ObjectAccessor.h"
#include "game/world/ObjectMgr.h"
#include "ObjectShapes.h"
#include "Projectile.h"
#include "ItemHelper.h"
#include "Item.h"
#include "Unit.h"
#include "Robot.h"

ItemBox::ItemBox() :
	m_openState(OPEN_STATE_LOCKED),
	m_templateId(0),
	m_lootId(0)
{
    m_type |= TypeMask::TYPEMASK_ITEMBOX;
    m_typeId = TypeID::TYPEID_ITEMBOX;
}

ItemBox::~ItemBox()
{

}

void ItemBox::update(NSTime diff)
{
	AttackableObject::update(diff);

	if (!this->isInWorld())
		return;

}

void ItemBox::addToWorld()
{
	if (this->isInWorld())
		return;

	AttackableObject::addToWorld();

	ObjectAccessor::addObject(this);
}

void ItemBox::removeFromWorld()
{
	if (!this->isInWorld())
		return;

	TileCoord currCoord(m_map->getMapData()->getMapSize(), this->getData()->getPosition());
	m_map->setTileClosed(currCoord, false);

	ObjectAccessor::removeObject(this);
	this->destroyForNearbyPlayers();

	AttackableObject::removeFromWorld();
}

void ItemBox::cleanupBeforeDelete()
{
	if (this->isInWorld())
		this->removeFromWorld();

	this->removeAllUnlockers();
	m_followerRefManager.clearReferences();

	AttackableObject::cleanupBeforeDelete();
}

void ItemBox::addUnlocker(Unit* unlocker)
{
	auto it = m_unlockers.find(unlocker);
	if (it == m_unlockers.end())
		m_unlockers.emplace(unlocker);
}

void ItemBox::removeUnlocker(Unit* unlocker)
{
	auto it = m_unlockers.find(unlocker);
	if (it != m_unlockers.end())
		m_unlockers.erase(it);
}

void ItemBox::removeAllUnlockers()
{
	while (!m_unlockers.empty())
	{
		auto it = m_unlockers.begin();
		if (!(*it)->removeUnlockTarget())
		{
			NS_LOG_ERROR("behaviors.item", "WORLD: Item has an unlocker that isn't unlocking it!");
			m_unlockers.erase(it);
		}
	}
}

bool ItemBox::isUnlocker(Unit* unit) const
{
	auto it = m_unlockers.find(unit);
	return it != m_unlockers.end();
}

void ItemBox::enterCollision(Projectile* proj, float precision)
{
	if (!proj->getLauncher().isValid() || !this->isLocked() || !this->getMap()->isInBattle())
		return;

	Unit* attacker = proj->getLauncher().getSource();

	// Calculate damage
	int32 damage = attacker->calcDamage(this, proj, precision);
	attacker->dealDamage(this, damage);
}

void ItemBox::receiveDamage(Unit* attacker, int32 damage)
{
	int32 newHealth = this->getData()->getHealth() - damage;
	this->getData()->setHealth(newHealth);
}

void ItemBox::opened(Unit* opener)
{
	NS_LOG_DEBUG("behaviors.itembox", "ItemBox (guid: 0x%08X) opened LootId: %d", this->getData()->getGuid().getRawValue(), m_lootId);
	this->dropLoot(opener);
	this->setOpenState(OPEN_STATE_OPENED);
}

bool ItemBox::loadData(SimpleItemBox const& simpleItemBox, BattleMap* map)
{
	DataItemBox* data = new DataItemBox();
	data->setGuid(ObjectGuid(GUIDTYPE_ITEMBOX, map->generateItemBoxSpawnId()));
	this->setData(data);
	
	return this->reloadData(simpleItemBox, map);
}

bool ItemBox::reloadData(SimpleItemBox const& simpleItemBox, BattleMap* map)
{
	DataItemBox* data = this->getData();
	if (!data)
		return false;

	m_templateId = simpleItemBox.templateId;
	m_lootId = simpleItemBox.lootId;

	ItemBoxTemplate const* tmpl = sObjectMgr->getItemBoxTemplate(m_templateId);
	NS_ASSERT(tmpl);

	data->setObjectSize(ITEMBOX_OBJECT_SIZE);
	data->setAnchorPoint(ITEMBOX_ANCHOR_POINT);
	data->setObjectRadiusInMap(ITEMBOX_OBJECT_RADIUS_IN_MAP);
	data->setLaunchCenter(ITEMBOX_LAUNCH_CENTER);

	if (!simpleItemBox.spawnPoint.isInvalid())
		data->setPosition(simpleItemBox.spawnPoint.computePosition(map->getMapData()->getMapSize()));
	data->setDirection(simpleItemBox.direction);
	data->setSpawnPoint(simpleItemBox.spawnPoint);

	data->setHealth(tmpl->maxHealth);
	data->setMaxHealth(tmpl->maxHealth);

	m_openState = OPEN_STATE_LOCKED;

	return true;
}

void ItemBox::setOpenState(OpenState state)
{
	m_openState = state;
	switch (state)
	{
	case OPEN_STATE_LOCKED:
		break;
	case OPEN_STATE_OPENED:
		this->removeAllAttackers();
		this->removeAllUnlockers();
		m_observerRefManager.clearReferences();
		m_followerRefManager.clearReferences();

		this->getData()->setHealth(0);
		break;
	default:
		break;
	}
}

void ItemBox::dropLoot(Unit* looter)
{
	ItemBoxItemList const* itemList = this->getMap()->getSpawnManager()->getItemBoxItemList(m_lootId);
	if (!itemList)
		return;

	// Initialize floor item position
	FloorItemPlace::Direction placeDir;
	switch (this->getData()->getDirection())
	{
	case DataItemBox::RIGHT_DOWN:
		placeDir = FloorItemPlace::RIGHT_DOWN;
		break;
	case DataItemBox::LEFT_DOWN:
		placeDir = FloorItemPlace::LEFT_DOWN;
		break;
	}
	TileCoord currCoord(m_map->getMapData()->getMapSize(), this->getData()->getPosition());
	FloorItemPlace itemPlace(m_map, currCoord, placeDir);

	// Create drop items
	for (auto it = itemList->begin(); it != itemList->end(); ++it)
	{
		ItemBoxItem const& item = *it;
		ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item.itemId);
		NS_ASSERT(tmpl);
		this->createItem(tmpl, itemPlace.nextEmptyTile(), item.count);
	}
}

void ItemBox::createItem(ItemTemplate const* tmpl, TileCoord const& spawnPoint, int32 count)
{
	SimpleItem simpleItem;
	simpleItem.templateId = tmpl->id;
	simpleItem.count = count;
	simpleItem.holder = this->getData()->getGuid();
	simpleItem.holderOrigin = this->getData()->getPosition();
	simpleItem.spawnPoint = spawnPoint;
	simpleItem.launchCenter = this->getData()->getLaunchCenter();
	simpleItem.dropDuration = random(ITEM_DROP_DELAY_MIN, ITEM_DROP_DELAY_MAX) + ITEM_PARABOLA_DURATION;

	Item* item = m_map->takeReusableObject<Item>();
	if (item)
		item->reloadData(simpleItem, m_map);
	else
	{
		item = new Item();
		item->loadData(simpleItem, m_map);
	}
	m_map->addToMap(item);
	item->updateObjectVisibility(true);
}