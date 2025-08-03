#include "DataItem.h"

#include "game/behaviors/Player.h"

DataItem::DataItem():
	m_itemId(0),
	m_position(Point::ZERO),
	m_count(0),
	m_holder(ObjectGuid::EMPTY),
	m_holderOrigin(Point::ZERO),
	m_launchCenter(Point::ZERO),
	m_launchRadiusInMap(0),
	m_dropDuration(0),
	m_dropElapsed(0)
{
	m_type |= DataTypeMask::DATA_TYPEMASK_ITEM;
	m_typeId = DataTypeID::DATA_TYPEID_ITEM;

	m_updateMask.setCount(SITEM_END);
}

DataItem::~DataItem()
{
}

void DataItem::writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const
{
	DataWorldObject::writeFields(updateMask, updateType, updateFlags, output);

	if (updateType == UPDATE_TYPE_CREATE)
	{
		if (m_itemId != 0)
		{
			Parcel::writeUInt32(m_itemId, output);
			updateMask.setBit(SITEM_FIELD_ITEMID);
		}

		if (m_position != Point::ZERO)
		{
			Parcel::writeFloat(m_position.x, output);
			Parcel::writeFloat(m_position.y, output);
			updateMask.setBit(SITEM_FIELD_POSITION);
		}

		if (m_count != 0)
		{
			Parcel::writeInt32(m_count, output);
			updateMask.setBit(SITEM_FIELD_COUNT);
		}

		if (m_holder != ObjectGuid::EMPTY)
		{
			Parcel::writeUInt32(m_holder.getRawValue(), output);
			updateMask.setBit(SITEM_FIELD_HOLDER);
		}

		if (m_holderOrigin != Point::ZERO)
		{
			Parcel::writeFloat(m_holderOrigin.x, output);
			Parcel::writeFloat(m_holderOrigin.y, output);
			updateMask.setBit(SITEM_FIELD_HOLDER_ORIGIN);
		}

		if (m_launchCenter != Point::ZERO)
		{
			Parcel::writeFloat(m_launchCenter.x, output);
			Parcel::writeFloat(m_launchCenter.y, output);
			updateMask.setBit(SITEM_FIELD_LAUNCH_CENTER);
		}

		if (m_launchRadiusInMap != 0)
		{
			Parcel::writeFloat(m_launchRadiusInMap, output);
			updateMask.setBit(SITEM_FIELD_LAUNCH_RADIUS_IN_MAP);
		}

		if (m_dropDuration != 0)
		{
			Parcel::writeInt32(m_dropDuration, output);
			updateMask.setBit(SITEM_FIELD_DROP_DURATION);
		}

		if (m_dropElapsed != 0)
		{
			Parcel::writeInt32(m_dropElapsed, output);
			updateMask.setBit(SITEM_FIELD_DROP_ELAPSED);
		}
	}
}

void DataItem::updateDataForPlayer(UpdateType updateType, Player* player)
{
}