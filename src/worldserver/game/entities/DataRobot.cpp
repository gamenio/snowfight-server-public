#include "DataRobot.h"

#include "game/behaviors/Player.h"

DataRobot::DataRobot() :
	m_target(ObjectGuid::EMPTY),
	m_stage(0),
	m_reactFlags(REACT_FLAG_NONE),
	m_proficiencyLevel(0),
	m_natureType(NATURE_CAREFUL),
	m_aiActionType(0),
	m_aiActionState(0)
{
	m_type |= DATA_TYPEMASK_ROBOT;
	m_typeId = DataTypeID::DATA_TYPEID_ROBOT;

	m_updateMask.setCount(SROBOT_END);
}


DataRobot::~DataRobot()
{
}

void DataRobot::writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const
{
	DataUnit::writeFields(updateMask, updateType, updateFlags, output);

	if ((updateType == UPDATE_TYPE_CREATE && m_target != ObjectGuid::EMPTY) || this->hasUpdatedField(SROBOT_FIELD_TARGET))
	{
		Parcel::writeUInt32(m_target.getRawValue(), output);
		updateMask.setBit(SROBOT_FIELD_TARGET);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_aiActionType != 0) || this->hasUpdatedField(SROBOT_FIELD_AIACTION_TYPE))
	{
		Parcel::writeUInt32(m_aiActionType, output);
		updateMask.setBit(SROBOT_FIELD_AIACTION_TYPE);
	}

	if ((updateType == UPDATE_TYPE_CREATE && m_aiActionState != 0) || this->hasUpdatedField(SROBOT_FIELD_AIACTION_STATE))
	{
		Parcel::writeUInt32(m_aiActionState, output);
		updateMask.setBit(SROBOT_FIELD_AIACTION_STATE);
	}
}

void DataRobot::updateDataForPlayer(UpdateType updateType, Player* player)
{
	if (updateType == UPDATE_TYPE_CREATE)
	{
		if (player->getSession())
		{
			int32 clientTime = player->getSession()->getClientNowTimeMillis();
			m_movementInfo.time = clientTime;

			if ((m_movementInfo.flags & MOVEMENT_FLAG_WALKING) != 0)
				m_moveSegment.time = clientTime;
		}
	}

}

void DataRobot::setTarget(ObjectGuid const& guid)
{
	if (m_target != guid)
	{
		m_target = guid;
		this->setUpdatedField(SROBOT_FIELD_TARGET);
		this->notifyDataUpdated();
	}
}

void DataRobot::setAIActionType(uint32 actionType)
{
	if (m_aiActionType != actionType)
	{
		m_aiActionType = actionType;
#if NS_DEBUG
		this->setUpdatedField(SROBOT_FIELD_AIACTION_TYPE);
		this->notifyDataUpdated();
#endif
	}
}

void DataRobot::setAIActionState(uint32 actionState)
{
	if (m_aiActionState != actionState)
	{
		m_aiActionState = actionState;
#if NS_DEBUG
		this->setUpdatedField(SROBOT_FIELD_AIACTION_STATE);
		this->notifyDataUpdated();
#endif
	}
}
