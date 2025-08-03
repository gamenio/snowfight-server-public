#include "game/server/WorldSession.h"

#include "game/entities/MovementInfo.h"
#include "game/server/protocol/OpcodeHandler.h"
#include "game/world/ObjectAccessor.h"
#include "game/behaviors/Player.h"
#include "game/behaviors/UnitLocator.h"
#include "game/grids/ObjectSearcher.h"


void broadcastMovementWorker(Player* source, uint16 opcode, MovementInfo& movement)
{
	if (source->getMap()->isInBattle() || source->getMap()->isBattleEnding())
	{
		// 向发送者发送心跳确认包
		if (opcode == MSG_MOVE_HEARTBEAT)
		{
			if (WorldSession* session = source->getSession())
			{
				WorldPacket packet(opcode);
				movement.time = session->getClientNowTimeMillis();
				session->packAndSend(std::move(packet), movement);
			}
		}
	}

	// 发送给周围能够看到发送者的玩家
	PlayerClientExistsObjectFilter filter(source);
	std::list<Player*> result;
	ObjectSearcher<Player, PlayerClientExistsObjectFilter> searcher(filter, result);

	source->visitNearbyObjectsInMaxVisibleRange(searcher);

	for (auto it = result.begin(); it != result.end(); ++it)
	{
		Player* target = *it;
		if (WorldSession* session = target->getSession())
		{
			WorldPacket packet(opcode);
			movement.time = session->getClientNowTimeMillis();
			session->packAndSend(std::move(packet), movement);
		}
	}
}

void broadcastLocationWorker(Player* source, LocationInfo& location)
{
	// 发送给周围能够看到定位器的玩家
	PlayerClientExistsLocatorFilter filter(source->getLocator());
	std::list<Player*> result;
	ObjectSearcher<Player, PlayerClientExistsLocatorFilter> searcher(filter, result);

	source->visitAllObjects(searcher);

	for (auto it = result.begin(); it != result.end(); ++it)
	{
		Player* target = *it;
		if (WorldSession* session = target->getSession())
		{
			WorldPacket packet(SMSG_RELOCATE_LOCATOR);
			location.time = session->getClientNowTimeMillis();
			session->packAndSend(std::move(packet), location);
		}

	}
}

void WorldSession::handleMovementInfo(WorldPacket& recvPacket)
{
	Player* mover = this->getPlayer();
	if (!mover->isInWorld() || mover->getMap()->isBattleEnded())
		return;
    
	MovementInfo movement;
	recvPacket.unpack(movement);

	uint16 opcode = recvPacket.getOpcode();
	DataPlayer* dPlayer = mover->getData();
    
	if (dPlayer->getMovementCounter() != movement.counter)
	{
		NS_LOG_WARN("world.handler.movement", "Player(%s)'s movement counter is wrong.(%d != %d)", movement.guid.toString().c_str(), dPlayer->getMovementCounter(), movement.counter);
		return;
	}

	float dist = dPlayer->getPosition().getDistance(movement.position);
	if (dist > MAX_STEP_LENGTH)
	{
		NS_LOG_WARN("world.handler.movement", "Player(%s) move step length is out of range.(%f > %f)", movement.guid.toString().c_str(), dist, MAX_STEP_LENGTH);
		this->kickPlayer();
		return;
	}

	MovementInfo prevMovementInfo = dPlayer->getMovementInfo();

	dPlayer->setMovementFlags(movement.flags);
	dPlayer->setOrientation(movement.orientation);
	broadcastMovementWorker(mover, opcode, movement);

	if (mover->getLocator())
	{
		DataUnitLocator* dLocator = mover->getLocator()->getData();
		dLocator->setPosition(movement.position);
		LocationInfo location = dLocator->getLocationInfo();
		broadcastLocationWorker(mover, location);
	}

	switch (opcode)
	{
	case MSG_MOVE_START:
		mover->addUnitState(UNIT_STATE_MOVING);
		break;
	case MSG_MOVE_STOP:
		mover->clearUnitState(UNIT_STATE_MOVING);
		break;
	}

	mover->updatePosition(movement.position);

	if ((movement.flags & MOVEMENT_FLAG_WALKING) != 0)
		// 记录上一个移动位置信息
		dPlayer->markMoveSegment(prevMovementInfo);
	else
		dPlayer->markMoveSegment(dPlayer->getMovementInfo());

    //NS_LOG_TRACE("world.handler.movement", "opcode: %s guid: %s movementinfo: %s", getOpcodeNameForLogging(recvPacket.getOpcode()).c_str(), movement.guid.toString().c_str(), movement.description().c_str());
}

