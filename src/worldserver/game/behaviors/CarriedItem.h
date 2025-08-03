#ifndef __CARRIED_ITEM_H__
#define __CARRIED_ITEM_H__

#include "game/entities/DataCarriedItem.h"
#include "game/entities/DataItem.h"
#include "game/maps/BattleMap.h"
#include "Object.h"

struct SimpleCarriedItem
{
	SimpleCarriedItem() :
		itemId(0),
		count(0),
		slot(SLOT_INVALID)
	{
	}

	uint16 itemId;
	int32 count;
	int32 slot;
};

class Unit;

class CarriedItem : public Object
{
public:
	CarriedItem();
    virtual ~CarriedItem();

	void addToWorld() override;
	void removeFromWorld() override;

	void setOwner(Unit* object) { m_owner = object; }
	Unit* getOwner() const { return m_owner; }

	PickupStatus canBeMergedPartlyWith(ItemTemplate const* itemTemplate) const;
	void drop();

	void buildValuesUpdate(PlayerUpdateMapType& updateMap);
	bool addToObjectUpdate() override;
	void removeFromObjectUpdate() override;

	bool loadData(SimpleCarriedItem const& simpleCarriedItem);
	virtual DataCarriedItem* getData() const override { return static_cast<DataCarriedItem*>(m_data); }

private:
	Unit* m_owner;
};


#endif // __CARRIED_ITEM_H__
