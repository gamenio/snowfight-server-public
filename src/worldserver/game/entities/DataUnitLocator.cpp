#include "DataUnitLocator.h"

DataUnitLocator::DataUnitLocator() :
	m_displayId(0),
	m_isAlive(true),
	m_moveSpeed(0)
{
	m_type |= DataTypeMask::DATA_TYPEMASK_UNIT_LOCATOR;
	m_typeId = DataTypeID::DATA_TYPEID_UNIT_LOCATOR;

	m_updateMask.setCount(SUNIT_LOCATOR_END);
}

DataUnitLocator::~DataUnitLocator()
{
}

void DataUnitLocator::setGuid(ObjectGuid const& guid)
{
	DataLocatorObject::setGuid(guid);
	m_locationInfo.guid = guid;
}

void DataUnitLocator::setAlive(bool isAlive)
{
	if (isAlive != m_isAlive)
	{
		m_isAlive = isAlive;
		this->setUpdatedField(SUNIT_LOCATOR_FIELD_IS_ALIVE);
		this->notifyDataUpdated();
	}
}

void DataUnitLocator::setMoveSpeed(int32 moveSpeed)
{
	if (moveSpeed != m_moveSpeed)
	{
		m_moveSpeed = moveSpeed;
		this->setUpdatedField(SUNIT_LOCATOR_FIELD_MOVE_SPEED);
		this->notifyDataUpdated();
	}
}

void DataUnitLocator::writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const
{
	DataLocatorObject::writeFields(updateMask, updateType, updateFlags, output);

	if (updateType == UPDATE_TYPE_CREATE)
	{
		m_locationInfo.writeToStream(output);
		updateMask.setBit(SUNIT_LOCATOR_FIELD_LOCATION);
	}

	if (updateType == UPDATE_TYPE_CREATE && m_displayId != 0)
	{
		Parcel::writeUInt32(m_displayId, output);
		updateMask.setBit(SUNIT_LOCATOR_FIELD_DISPLAYID);
	}

	if ((updateType == UPDATE_TYPE_CREATE && !m_isAlive) || this->hasUpdatedField(SUNIT_LOCATOR_FIELD_IS_ALIVE))
	{
		Parcel::writeBool(m_isAlive, output);
		updateMask.setBit(SUNIT_LOCATOR_FIELD_IS_ALIVE);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_moveSpeed != 0) || this->hasUpdatedField(SUNIT_LOCATOR_FIELD_MOVE_SPEED))
	{
		Parcel::writeInt32(m_moveSpeed, output);
		updateMask.setBit(SUNIT_LOCATOR_FIELD_MOVE_SPEED);
	}
}