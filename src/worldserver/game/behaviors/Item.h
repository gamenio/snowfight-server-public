#ifndef __ITEM_H__
#define __ITEM_H__

#include "game/entities/DataItem.h"
#include "game/movement/generators/FollowerReference.h"
#include "game/movement/generators/FollowerRefManager.h"
#include "game/collection/ItemCollisionReference.h"
#include "game/collection/InterestedRefManager.h"
#include "WorldObject.h"
#include "ItemHelper.h"

// Delay time for item drop. Unit: milliseconds
#define ITEM_DROP_DELAY_MIN					0		
#define ITEM_DROP_DELAY_MAX					200

struct SimpleItem
{
	SimpleItem() :
		templateId(0),
		spawnInfoId(0),
		count(0),
		holder(ObjectGuid::EMPTY),
		launchRadiusInMap(0),
		spawnPoint(TileCoord::INVALID),
		dropDuration(0)
	{}

	uint32 templateId;
	uint32 spawnInfoId;
	int32 count;
	ObjectGuid holder;
	Point holderOrigin;
	Point launchCenter;
	float launchRadiusInMap;
	TileCoord spawnPoint;
	int32 dropDuration;
};

enum ItemState
{
	ITEM_STATE_NONE,
	ITEM_STATE_ACTIVATING,
	ITEM_STATE_ACTIVE,
	ITEM_STATE_INACTIVE,
};

class CarriedItem;

class Item : public WorldObject, public GridObject<Item>
{
public:
    Item();
    virtual ~Item();

	void update(NSTime diff) override;
	void addToWorld() override;
	void removeFromWorld() override;

	void cleanupBeforeDelete() override;

	bool isActive() const { return m_itemState == ITEM_STATE_ACTIVE; }

	void addCollector(Robot* collector);
	void removeCollector(Robot* collector);
	void removeAllCollectors();
	int32 getCollectorCount() const { return (int32)m_collectors.size(); }
	bool isCollector(Robot* robot) const;

	void addPicker(Unit* picker);
	void removePicker(Unit* picker);
	void removeAllPickers();
	int32 getPickerCount() const { return (int32)m_pickers.size(); }
	bool isPicker(Unit* unit) const;

	void receiveBy(Unit* receiver);
	bool canBeMergedWith(CarriedItem* carriedItem);

	bool canDetect(Unit* target) const;
	void testCollision(Unit* target);

	FollowerRefManager<Item>* getFollowerRefManager() { return &m_followerRefManager; }
	InterestedRefManager* getInterestedRefManager() { return &m_interestedRefManager; }

	bool loadData(SimpleItem const& simpleItem, BattleMap* map);
	bool reloadData(SimpleItem const& simpleItem, BattleMap* map);
	virtual DataItem* getData() const override { return static_cast<DataItem*>(m_data); }

private:
	bool isCollideWith(Unit* target);

	void addCollidedObject(Unit* object);
	void removeCollidedObject(Unit* object);
	void removeAllCollidedObjects();

	void respawn();
	void resetRespawnTimer();

	bool inactivate();
	void activate();
	void setItemState(ItemState state);
	void resetActivationTimer();

	uint32 m_spawnInfoId;

	ItemState m_itemState;
	int64 m_activationTimer;
	int64 m_respawnTimer;

	std::unordered_map<ObjectGuid, ItemCollisionReference*> m_collidedObjects;
	std::unordered_set<Unit*> m_pickers;
	std::unordered_set<Robot*> m_collectors;
	FollowerRefManager<Item> m_followerRefManager;
	InterestedRefManager m_interestedRefManager;
};


#endif // __ITEM_H__
