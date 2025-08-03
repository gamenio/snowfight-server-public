#include "DataPlayer.h"

#include "game/behaviors/Object.h"
#include "game/behaviors/Player.h"

DataPlayer::DataPlayer() : 
	m_isGM(false),
	m_pickupTarget(ObjectGuid::EMPTY),
	m_viewport(Size::ZERO),
	m_visibleRange(Size::ZERO),
	m_lang(LANG_enUS),
	m_controllerType(CONTROLLER_TYPE_UNKNOWN),
	m_isNewPlayer(false),
	m_isTrainee(false),
	m_rewardStage(0),
	m_dailyRewardDays(0),
	m_combatGrade(0),
	m_property(0),
	m_statStageList(),
	m_selectedMapId(0)
{
	m_type |= DATA_TYPEMASK_PLAYER;
	m_typeId = DataTypeID::DATA_TYPEID_PLAYER;

	m_updateMask.setCount(SPLAYER_END);
}

DataPlayer::~DataPlayer()
{
}

void DataPlayer::writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const
{
	DataUnit::writeFields(updateMask, updateType, updateFlags, output);

	if ((updateType == UPDATE_TYPE_CREATE && m_isGM) || this->hasUpdatedField(SPLAYER_FIELD_IS_GM))
	{
		Parcel::writeBool(m_isGM, output);
		updateMask.setBit(SPLAYER_FIELD_IS_GM);
	}

	// 更新玩家自己
	if ((updateFlags & UPDATE_FLAG_SELF) != 0)
	{
		if ((updateType == UPDATE_TYPE_CREATE && m_experience != 0) || this->hasUpdatedField(SPLAYER_FIELD_EXPERIENCE))
		{
			Parcel::writeInt32(m_experience, output);
			updateMask.setBit(SPLAYER_FIELD_EXPERIENCE);
		}

		if ((updateType == UPDATE_TYPE_CREATE && m_nextLevelXP != 0) || this->hasUpdatedField(SPLAYER_FIELD_NEXTLEVEL_XP))
		{
			Parcel::writeInt32(m_nextLevelXP, output);
			updateMask.setBit(SPLAYER_FIELD_NEXTLEVEL_XP);
		}

		if ((updateType == UPDATE_TYPE_CREATE && m_money != 0) || this->hasUpdatedField(SPLAYER_FIELD_MONEY))
		{
			Parcel::writeInt32(m_money, output);
			updateMask.setBit(SPLAYER_FIELD_MONEY);
		}

		// 物品
		for (int32 slot = 0; slot < UNIT_SLOTS_COUNT; ++slot)
		{
			if ((updateType == UPDATE_TYPE_CREATE && m_items[slot] != ObjectGuid::EMPTY) || this->hasUpdatedField(SPLAYER_FIELD_ITEM_HEAD + slot))
			{
				Parcel::writeUInt32(m_items[slot].getRawValue(), output);
				updateMask.setBit(SPLAYER_FIELD_ITEM_HEAD + slot);
			}
		}

		if ((updateType == UPDATE_TYPE_CREATE && m_concealmentState != CONCEALMENT_STATE_EXPOSED) || this->hasUpdatedField(SPLAYER_FIELD_CONCEALMENT_STATE))
		{
			Parcel::writeUInt32(m_concealmentState, output);
			updateMask.setBit(SPLAYER_FIELD_CONCEALMENT_STATE);
		}

		if ((updateType == UPDATE_TYPE_CREATE && m_pickupDuration != 0))
		{
			Parcel::writeInt32(m_pickupDuration, output);
			updateMask.setBit(SPLAYER_FIELD_PICKUP_DURATION);
		}

		if ((updateType == UPDATE_TYPE_CREATE && m_pickupTarget != ObjectGuid::EMPTY) || this->hasUpdatedField(SPLAYER_FIELD_PICKUP_TARGET))
		{
			Parcel::writeUInt32(m_pickupTarget.getRawValue(), output);
			updateMask.setBit(SPLAYER_FIELD_PICKUP_TARGET);
		}

		if ((updateType == UPDATE_TYPE_CREATE && m_killCount != 0) || this->hasUpdatedField(SPLAYER_FIELD_KILL_COUNT))
		{
			Parcel::writeInt32(m_killCount, output);
			updateMask.setBit(SPLAYER_FIELD_KILL_COUNT);
		}
	}
}

void DataPlayer::setViewport(Size const& size)
{
	m_viewport = size;
	m_visibleRange.setSize(size.width + MAX_SIZE_OF_OBJECT, size.height + MAX_SIZE_OF_OBJECT);
}

void DataPlayer::setGM(bool isGM)
{
	if (m_isGM != isGM)
	{
		m_isGM = isGM;
		this->setUpdatedField(SPLAYER_FIELD_IS_GM);
		this->notifyDataUpdated();
	}
}

void DataPlayer::updateDataForPlayer(UpdateType updateType, Player* player)
{
	if (updateType == UPDATE_TYPE_CREATE)
	{
		if (player->getSession())
		{
			int32 clientTime = player->getSession()->getClientNowTimeMillis();
			m_movementInfo.time = clientTime;
			m_staminaInfo.time = clientTime;
		}
	}
}

void DataPlayer::setKillCount(int32 count)
{
	if (m_killCount != count)
	{
		m_killCount = count;
		this->setUpdatedField(SPLAYER_FIELD_KILL_COUNT);
		this->notifyDataUpdated();
	}
}

void DataPlayer::setItem(int32 slot, ObjectGuid const& guid)
{
	if (m_items[slot] != guid)
	{
		m_items[slot] = guid;
		this->setUpdatedField(SPLAYER_FIELD_ITEM_HEAD + slot);
		this->notifyDataUpdated();
	}
}

void DataPlayer::setConcealmentState(ConcealmentState state)
{
	if (m_concealmentState != state)
	{
		m_concealmentState = state;
		this->setUpdatedField(SPLAYER_FIELD_CONCEALMENT_STATE);
		this->notifyDataUpdated();
	}
}

void DataPlayer::setPickupDuration(int32 duration)
{
	if (m_pickupDuration != duration)
	{
		m_pickupDuration = duration;
		this->setUpdatedField(SPLAYER_FIELD_PICKUP_DURATION);
		this->notifyDataUpdated();
	}
}

void DataPlayer::setPickupTarget(ObjectGuid const& guid)
{
	if (m_pickupTarget != guid)
	{
		m_pickupTarget = guid;
		this->setUpdatedField(SPLAYER_FIELD_PICKUP_TARGET);
		this->notifyDataUpdated();
	}
}

void DataPlayer::setExperience(int32 xp)
{
	if (m_experience != xp)
	{
		m_experience = xp;
		this->setUpdatedField(SPLAYER_FIELD_EXPERIENCE);
		this->notifyDataUpdated();
	}
}

void DataPlayer::setNextLevelXP(int32 xp)
{
	if (m_nextLevelXP != xp)
	{
		m_nextLevelXP = xp;
		this->setUpdatedField(SPLAYER_FIELD_NEXTLEVEL_XP);
		this->notifyDataUpdated();
	}
}

void DataPlayer::setMoney(int32 money)
{
	if (m_money != money)
	{
		m_money = money;
		this->setUpdatedField(SPLAYER_FIELD_MONEY);
		this->notifyDataUpdated();
	}
}