#include "Object.h"

#include "game/server/protocol/pb/DestroyObject.pb.h"

#include "debugging/Errors.h"
#include "game/behaviors/Player.h"

Object::Object() :
	m_typeId(TypeID::TYPEID_OBJECT),
	m_type(TypeMask::TYPEMASK_OBJECT),
	m_data(nullptr),
	m_objectUpdated(false),
	m_isInWorld(false)
{

}

Object::~Object()
{
	NS_ASSERT_FATAL(!m_isInWorld, "%s deleted but still in world!", m_data->getGuid().toString().c_str());
	NS_ASSERT_FATAL(!m_objectUpdated, "%s deleted but still in update list!", m_data->getGuid().toString().c_str());

	if (m_data)
	{
		delete m_data;
		m_data = nullptr;
	}

}

void Object::setData(DataBasic* data)
{
	if (m_data != data)
	{
		if (m_data)
		{
			delete m_data;
			m_data = nullptr;
		}
		data->setDataObserver(this);
		m_data = data;
	}
}

void Object::addToWorld()
{
	if (m_isInWorld)
		return;

	m_isInWorld = true;
	this->clearUpdateMask(false);
}


void Object::removeFromWorld()
{
	if (!m_isInWorld)
		return;

	m_isInWorld = false;
	this->clearUpdateMask(true);
}

void Object::sendValuesUpdateToPlayer(Player* player) const
{
	if (!player->getSession())
		return;

	UpdateObject update;
	this->buildValuesUpdateBlockForPlayer(player, &update);

	WorldPacket packet(SMSG_UPDATE_OBJECT);
	player->getSession()->packAndSend(std::move(packet), update);
}

void Object::sendCreateUpdateToPlayer(Player* player) const
{
	if (!player->getSession())
		return;

	UpdateObject update;
	this->buildCreateUpdateBlockForPlayer(player, &update);

	WorldPacket packet(SMSG_UPDATE_OBJECT);
	player->getSession()->packAndSend(std::move(packet), update);
}

void Object::sendOutOfRangeUpdateToPlayer(Player* player) const
{
	if (!player->getSession())
		return;

	UpdateObject update;
	WorldPacket packet(SMSG_UPDATE_OBJECT);
	update.addOutOfRangeGUID(m_data->getGuid());
	player->getSession()->packAndSend(std::move(packet), update);
}

void Object::sendDestroyToPlayer(Player* player) const
{
	if (!player->getSession())
		return;

	WorldPacket packet(SMSG_DESTROY_OBJECT);
	DestroyObject message;
	message.set_guid(m_data->getGuid().getRawValue());
	player->getSession()->packAndSend(std::move(packet), message);
}

void Object::buildCreateUpdateBlockForPlayer(Player* player, UpdateObject* update) const
{
	UpdateType updateType = UPDATE_TYPE_CREATE;
	uint32 updateFlags = UPDATE_FLAG_NONE;

	if (player == this)
		updateFlags |= UPDATE_FLAG_SELF;

	m_data->updateDataForPlayer(updateType, player);
	update->addData(m_data, updateType, updateFlags);
}

void Object::buildValuesUpdateBlockForPlayer(Player* player, UpdateObject* update) const
{
	UpdateType updateType = UPDATE_TYPE_VALUES;
	uint32 updateFlags = UPDATE_FLAG_NONE;

	if (player == this)
		updateFlags |= UPDATE_FLAG_SELF;

	m_data->updateDataForPlayer(updateType, player);
	update->addData(m_data, updateType, updateFlags);
}

void Object::buildOutOfRangeUpdateBlock(UpdateObject* update) const
{
	update->addOutOfRangeGUID(m_data->getGuid());
}

void Object::buildValuesUpdateForPlayerInMap(Player* player, PlayerUpdateMapType& updateMap) const
{
	auto iter = updateMap.find(player);
	if (iter == updateMap.end())
	{
		std::pair<PlayerUpdateMapType::iterator, bool> p = updateMap.emplace(player, UpdateObject());
		NS_ASSERT(p.second);
		iter = p.first;
	}
	this->buildValuesUpdateBlockForPlayer(iter->first, &iter->second);
}

void Object::cleanupAfterUpdate()
{
	this->clearUpdateMask(false);
}

void Object::addToObjectUpdateIfNeeded()
{
	if (!m_objectUpdated)
	{
		if(this->addToObjectUpdate())
			m_objectUpdated = true;
	}
}

void Object::clearUpdateMask(bool remove)
{
	if(m_data)
		m_data->clearUpdateFlags();

	if (m_objectUpdated)
	{
		if (remove)
			this->removeFromObjectUpdate();

		m_objectUpdated = false;
	}
}