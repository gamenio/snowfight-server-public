#include "DataItemBox.h"

DataItemBox::DataItemBox():
	m_launchCenter(Point::ZERO),
	m_position(Point::ZERO),
	m_direction(LEFT_DOWN),
	m_health(0),
	m_maxHealth(0)
{
	m_type |= DataTypeMask::DATA_TYPEMASK_ITEMBOX;
	m_typeId = DataTypeID::DATA_TYPEID_ITEMBOX;

	m_updateMask.setCount(SITEMBOX_END);
}

DataItemBox::~DataItemBox()
{
}

void DataItemBox::setHealth(int32 health)
{
	if (m_health != health)
	{
		m_health = health;
		this->setUpdatedField(SITEMBOX_FIELD_HEALTH);
		this->notifyDataUpdated();
	}
}

void DataItemBox::setMaxHealth(int32 maxHealth)
{
	if (m_maxHealth != maxHealth)
	{
		m_maxHealth = maxHealth;
		this->setUpdatedField(SITEMBOX_FIELD_MAX_HEALTH);
		this->notifyDataUpdated();
	}
}

void DataItemBox::writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const
{
	DataWorldObject::writeFields(updateMask, updateType, updateFlags, output);

	if (updateType == UPDATE_TYPE_CREATE)
	{
		if (m_position != Point::ZERO)
		{
			Parcel::writeFloat(m_position.x, output);
			Parcel::writeFloat(m_position.y, output);
			updateMask.setBit(SITEMBOX_FIELD_POSITION);
		}

		if (m_direction != LEFT_DOWN)
		{
			Parcel::writeUInt32(m_direction, output);
			updateMask.setBit(SITEMBOX_FIELD_DIRECTION);
		}
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_health != 0) || this->hasUpdatedField(SITEMBOX_FIELD_HEALTH))
	{
		Parcel::writeInt32(m_health, output);
		updateMask.setBit(SITEMBOX_FIELD_HEALTH);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_maxHealth != 0) || this->hasUpdatedField(SITEMBOX_FIELD_MAX_HEALTH))
	{
		Parcel::writeInt32(m_maxHealth, output);
		updateMask.setBit(SITEMBOX_FIELD_MAX_HEALTH);
	}
}


