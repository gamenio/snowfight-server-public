#include "DataCarriedItem.h"

DataCarriedItem::DataCarriedItem():
	m_itemId(0),
	m_level(0),
	m_count(0),
	m_stackable(ITEM_STACK_NON_STACKABLE),
	m_slot(SLOT_INVALID),
	m_cooldownDuration(0)
{
	m_type |= DataTypeMask::DATA_TYPEMASK_CARRIED_ITEM;
	m_typeId = DataTypeID::DATA_TYPEID_CARRIED_ITEM;

	m_updateMask.setCount(SCARRIEDITEM_END);
}

DataCarriedItem::~DataCarriedItem()
{
}

void DataCarriedItem::setCount(int32 count)
{
	if (count != m_count)
	{
		m_count = count;
		this->setUpdatedField(SCARRIEDITEM_FIELD_COUNT);
		this->notifyDataUpdated();
	}
}

void DataCarriedItem::writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const
{
	if (updateType == UPDATE_TYPE_CREATE && m_itemId != 0)
	{
		Parcel::writeUInt32(m_itemId, output);
		updateMask.setBit(SCARRIEDITEM_FIELD_ITEMID);
	}

	if (updateType == UPDATE_TYPE_CREATE && m_level != 0)
	{
		Parcel::writeInt32(m_level, output);
		updateMask.setBit(SCARRIEDITEM_FIELD_LEVEL);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_count != 0) || this->hasUpdatedField(SCARRIEDITEM_FIELD_COUNT))
	{
		Parcel::writeInt32(m_count, output);
		updateMask.setBit(SCARRIEDITEM_FIELD_COUNT);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_stackable != ITEM_STACK_NON_STACKABLE))
	{
		Parcel::writeInt32(m_stackable, output);
		updateMask.setBit(SCARRIEDITEM_FIELD_STACKABLE);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_cooldownDuration != 0))
	{
		Parcel::writeInt32(m_cooldownDuration, output);
		updateMask.setBit(SCARRIEDITEM_FIELD_COOLDOWN_DURATION);
	}
}

void DataCarriedItem::updateDataForPlayer(UpdateType updateType, Player* player)
{
}

