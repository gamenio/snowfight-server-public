#include "DataUnit.h"

#include "logging/Log.h"
#include "game/utils/MathTools.h"
#include "game/behaviors/Object.h"
#include "MovementInfo.h"

DataUnit::DataUnit() : 
	m_name(""),
	m_displayId(0),
	m_health(0),
	m_maxHealth(0),
	m_baseMaxHealth(0),
	m_healthRegenRate(0),
	m_baseHealthRegenRate(0),
	m_moveSpeed(0),
	m_baseMoveSpeed(0),
	m_attackRange(0),
	m_baseAttackRange(0),
	m_damage(0),
	m_baseDamage(0),
	m_defense(0),
	m_attackTakesStamina(0),
	m_baseAttackTakesStamina(0),
	m_baseMaxStamina(0),
	m_baseStaminaRegenRate(0),
	m_baseChargeConsumesStamina(0),
	m_level(0),
	m_experience(0),
	m_nextLevelXP(0),
	m_launchCenter(Point::ZERO),
	m_launchRadiusInMap(0),
	m_money(0),
	m_combatPower(0),
	m_unitFlags(0),
	m_smiley(SMILEY_NONE),
	m_concealmentState(CONCEALMENT_STATE_EXPOSED),
	m_pickupDuration(0),
	m_magicBeanCount(0),
	m_killCount(0)
{
	m_type |= DataTypeMask::DATA_TYPEMASK_UNIT;
	m_typeId = DataTypeID::DATA_TYPEID_UNIT;
}

DataUnit::~DataUnit()
{
}

void DataUnit::writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const
{
	DataWorldObject::writeFields(updateMask, updateType, updateFlags, output);

	if ((updateType == UPDATE_TYPE_CREATE && m_health != 0) || this->hasUpdatedField(SUNIT_FIELD_HEALTH))
	{
		Parcel::writeInt32(m_health, output);
		updateMask.setBit(SUNIT_FIELD_HEALTH);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_maxHealth != 0) || this->hasUpdatedField(SUNIT_FIELD_MAX_HEALTH))
	{
		Parcel::writeInt32(m_maxHealth, output);
		updateMask.setBit(SUNIT_FIELD_MAX_HEALTH);
	}

	if (updateType == UPDATE_TYPE_CREATE)
	{
		m_movementInfo.writeToStream(output);
		if ((m_movementInfo.flags & MOVEMENT_FLAG_WALKING) != 0)
			m_moveSegment.writeToStream(output);
		updateMask.setBit(SUNIT_FIELD_MOVEMENT_INFO);
	}

	if (updateType == UPDATE_TYPE_CREATE)
	{
		m_staminaInfo.writeToStream(output);
		updateMask.setBit(SUNIT_FIELD_STAMINA_INFO);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_moveSpeed != 0) || this->hasUpdatedField(SUNIT_FIELD_MOVE_SPEED))
	{
		Parcel::writeInt32(m_moveSpeed, output);
		updateMask.setBit(SUNIT_FIELD_MOVE_SPEED);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_attackRange != 0) || this->hasUpdatedField(SUNIT_FIELD_ATTACK_RANGE))
	{
		Parcel::writeFloat(m_attackRange, output);
		updateMask.setBit(SUNIT_FIELD_ATTACK_RANGE);
	}

	if (updateType == UPDATE_TYPE_CREATE && m_attackTakesStamina != 0)
	{
		Parcel::writeInt32(m_attackTakesStamina, output);
		updateMask.setBit(SUNIT_FIELD_ATTACK_TAKES_STAMINA);
	}

	if (updateType == UPDATE_TYPE_CREATE && m_displayId != 0)
	{
		Parcel::writeUInt32(m_displayId, output);
		updateMask.setBit(SUNIT_FIELD_DISPLAYID);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_level != 0) || this->hasUpdatedField(SUNIT_FIELD_LEVEL))
	{
		Parcel::writeInt32(m_level, output);
		updateMask.setBit(SUNIT_FIELD_LEVEL);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_unitFlags != 0) || this->hasUpdatedField(SUNIT_FIELD_UNIT_FLAGS))
	{
		Parcel::writeUInt32(m_unitFlags, output);
		updateMask.setBit(SUNIT_FIELD_UNIT_FLAGS);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_smiley != SMILEY_NONE) || this->hasUpdatedField(SUNIT_FIELD_SMILEY))
	{
		Parcel::writeInt32(m_smiley, output);
		updateMask.setBit(SUNIT_FIELD_SMILEY);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_magicBeanCount != 0) || this->hasUpdatedField(SUNIT_FIELD_MAGIC_BEAN_COUNT))
	{
		Parcel::writeInt32(m_magicBeanCount, output);
		updateMask.setBit(SUNIT_FIELD_MAGIC_BEAN_COUNT);
	}
}

void DataUnit::setGuid(ObjectGuid const& guid)
{
	DataWorldObject::setGuid(guid); 

	m_movementInfo.guid = guid;
	m_staminaInfo.guid = guid;
}

void DataUnit::setLevel(uint8 level)
{
	if (m_level != level)
	{
		m_level = level;
		this->setUpdatedField(SUNIT_FIELD_LEVEL);
		this->notifyDataUpdated();
	}
}

void DataUnit::setOrientation(float rad)
{
	if (m_movementInfo.orientation != rad)
	{
		m_movementInfo.orientation = rad;
	}
}

void DataUnit::setAttackRange(float range)
{
	if (m_attackRange != range)
	{
		m_attackRange = range;
		this->setUpdatedField(SUNIT_FIELD_ATTACK_RANGE);
		this->notifyDataUpdated();

	}

}

void DataUnit::setMoveSpeed(int32 moveSpeed)
{
	if (m_moveSpeed != moveSpeed)
	{
		m_moveSpeed = moveSpeed;
		this->setUpdatedField(SUNIT_FIELD_MOVE_SPEED);
		this->notifyDataUpdated();
	}
}

void DataUnit::setHealth(int32 health)
{
	if (m_health!= health)
	{
		m_health = health;
		this->setUpdatedField(SUNIT_FIELD_HEALTH);
		this->notifyDataUpdated();
	}
}

void DataUnit::setMaxHealth(int32 maxHealth)
{
	if (m_maxHealth != maxHealth)
	{
		m_maxHealth = maxHealth;
		this->setUpdatedField(SUNIT_FIELD_MAX_HEALTH);
		this->notifyDataUpdated();
	}
}

void DataUnit::addUnitFlag(uint32 flag)
{
	uint32 newFlags = m_unitFlags | flag;
	if (m_unitFlags != newFlags)
	{
		m_unitFlags = newFlags;
		this->setUpdatedField(SUNIT_FIELD_UNIT_FLAGS);
		this->notifyDataUpdated();
	}
}

void DataUnit::clearUnitFlag(uint32 flag)
{
	uint32 newFlags = m_unitFlags & ~flag;
	if (m_unitFlags != newFlags)
	{
		m_unitFlags = newFlags;
		this->setUpdatedField(SUNIT_FIELD_UNIT_FLAGS);
		this->notifyDataUpdated();
	}
}

bool DataUnit::hasUnitFlag(uint32 flag) const
{
	return (m_unitFlags & flag) != 0;
}

void DataUnit::setUnitFlags(uint32 flags)
{
	if (m_unitFlags != flags)
	{
		m_unitFlags = flags;
		this->setUpdatedField(SUNIT_FIELD_UNIT_FLAGS);
		this->notifyDataUpdated();
	}
}

void DataUnit::setSmiley(uint16 code)
{
	if (m_smiley != code)
	{
		m_smiley = code;
		this->setUpdatedField(SUNIT_FIELD_SMILEY);
		this->notifyDataUpdated();
	}
}

void DataUnit::setMagicBeanCount(int32 count)
{
	if (m_magicBeanCount != count)
	{
		m_magicBeanCount = count;
		this->setUpdatedField(SUNIT_FIELD_MAGIC_BEAN_COUNT);
		this->notifyDataUpdated();
	}
}
