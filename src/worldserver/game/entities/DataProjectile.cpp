#include "DataProjectile.h"

#include "game/behaviors/Player.h"
#include "utilities/TimeUtil.h"

DataProjectile::DataProjectile():
	m_launcher(ObjectGuid::EMPTY),
	m_launcherOrigin(Point::ZERO),
	m_attackRange(0),
	m_launchCenter(Point::ZERO),
	m_launchRadiusInMap(0),
	m_position(Point::ZERO),
	m_orientation(0),
	m_projType(PROJECTILE_TYPE_NORMAL),
	m_damageBonusRatio(0),
	m_elapsed(0),
	m_duration(0),
	m_attackCounter(0),
	m_consumedStamina(0),
	m_chargedStamina(0),
	m_scale(0),
	m_attackInfoCounter(0),
	m_status(LAUNCHSTATUS_NONE)
{
	m_type |= DataTypeMask::DATA_TYPEMASK_PROJECTILE;
	m_typeId = DataTypeID::DATA_TYPEID_PROJECTILE;

	m_updateMask.setCount(SPROJECTILE_END);
}

DataProjectile::~DataProjectile()
{
}

void DataProjectile::writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const
{
	DataWorldObject::writeFields(updateMask, updateType, updateFlags, output);

	if (updateType == UPDATE_TYPE_CREATE)
	{
		if (m_launcher != ObjectGuid::EMPTY)
		{
			Parcel::writeUInt32(m_launcher.getRawValue(), output);
			updateMask.setBit(SPROJECTILE_FIELD_LAUNCHER);
		}
 
		if (m_launcherOrigin != Point::ZERO)
		{
			Parcel::writeFloat(m_launcherOrigin.x, output);
			Parcel::writeFloat(m_launcherOrigin.y, output);
			updateMask.setBit(SPROJECTILE_FIELD_LAUNCHER_ORIGIN);
		}
 
		if (m_attackRange != 0)
		{
			Parcel::writeFloat(m_attackRange, output);
			updateMask.setBit(SPROJECTILE_FIELD_ATTACK_RANGE);
		}
 
		if (m_launchCenter != Point::ZERO)
		{
			Parcel::writeFloat(m_launchCenter.x, output);
			Parcel::writeFloat(m_launchCenter.y, output);
			updateMask.setBit(SPROJECTILE_FIELD_LAUNCH_CENTER);
		}
 
		if (m_launchRadiusInMap != 0)
		{
			Parcel::writeFloat(m_launchRadiusInMap, output);
			updateMask.setBit(SPROJECTILE_FIELD_LAUNCH_RADIUS_IN_MAP);
		}

		if (m_orientation != 0)
		{
			Parcel::writeFloat(m_orientation, output);
			updateMask.setBit(SPROJECTILE_FIELD_ORIENTATION);
		}

		if (m_elapsed != 0)
		{
			Parcel::writeInt32(m_elapsed, output);
			updateMask.setBit(SPROJECTILE_FIELD_ELAPSED);
		}

		if (m_duration != 0)
		{
			Parcel::writeInt32(m_duration, output);
			updateMask.setBit(SPROJECTILE_FIELD_DURATION);
		}

		if (m_attackCounter != 0)
		{
			Parcel::writeUInt32(m_attackCounter, output);
			updateMask.setBit(SPROJECTILE_FIELD_ATTACK_COUNTER);
		}

		if (m_consumedStamina != 0)
		{
			Parcel::writeInt32(m_consumedStamina, output);
			updateMask.setBit(SPROJECTILE_FIELD_CONSUMED_STAMINA);
		}

		if (m_scale != 0)
		{
			Parcel::writeFloat(m_scale, output);
			updateMask.setBit(SPROJECTILE_FIELD_SCALE);
		}

		if (m_attackInfoCounter != 0)
		{
			Parcel::writeUInt32(m_attackInfoCounter, output);
			updateMask.setBit(SPROJECTILE_FIELD_ATTACK_INFO_COUNTER);
		}

		if (m_status != LAUNCHSTATUS_NONE)
		{
			Parcel::writeInt32(m_status, output);
			updateMask.setBit(SPROJECTILE_FIELD_FLAG);
		}
	}
}

void DataProjectile::updateDataForPlayer(UpdateType updateType, Player* player)
{
}
