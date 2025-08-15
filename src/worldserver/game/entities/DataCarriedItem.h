#ifndef __DATA_CARRIED_ITEM_H__
#define __DATA_CARRIED_ITEM_H__

#include "Common.h"
#include "DataBasic.h"
#include "DataItem.h"

enum ItemUseStatus
{
	ITEM_USE_STATUS_OK,
	ITEM_USE_STATUS_FAILED,
};

class DataCarriedItem: public DataBasic
{
public:
	DataCarriedItem();
	virtual ~DataCarriedItem();

	uint32 getItemId() const { return m_itemId; }
	void setItemId(uint32 id) { m_itemId = id; }

	void setLevel(uint8 level) { m_level = level; }
	uint8 getLevel() const { return m_level; }

	void setCount(int32 count);
	int32 getCount() const { return m_count; }

	void setStackable(int32 stackable) { m_stackable = stackable; }
	int32 getStackable() const { return m_stackable; }

	void setSlot(int32 slot) { m_slot = slot; }
	int32 getSlot() const { return m_slot; }

	// Cooldown time for the item. Unit: milliseconds
	void setCooldownDuration(int32 duration) { m_cooldownDuration = duration; }
	int32 getCooldownDuration() const { return m_cooldownDuration; }
    
	void writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const override;
	void updateDataForPlayer(UpdateType updateType, Player* player) override;

private:
	uint32 m_itemId;
	uint8 m_level;
	int32 m_count;
	int32 m_stackable;
	int32 m_slot;
	int32 m_cooldownDuration;
};

#endif // __DATA_CARRIED_ITEM_H__