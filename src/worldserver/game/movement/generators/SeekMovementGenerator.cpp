#include "SeekMovementGenerator.h"

#include "logging/Log.h"
#include "game/server/protocol/Opcode.h"

#define OFFSET_TOP(coord)					TileCoord(coord.x, coord.y - 1)
#define OFFSET_BOTTOM(coord)				TileCoord(coord.x, coord.y + 1)
#define OFFSET_LEFT(coord)					TileCoord(coord.x - 1, coord.y)
#define OFFSET_RIGHT(coord)					TileCoord(coord.x + 1, coord.y)
#define OFFSET_LEFT_TOP(coord)				TileCoord(coord.x - 1, coord.y - 1)
#define OFFSET_LEFT_BOTTOM(coord)			TileCoord(coord.x - 1, coord.y + 1)
#define OFFSET_RIGHT_TOP(coord)				TileCoord(coord.x + 1, coord.y - 1)
#define OFFSET_RIGHT_BOTTOM(coord)			TileCoord(coord.x + 1, coord.y + 1)

SeekMovementGenerator::SeekMovementGenerator(Robot* owner, TileCoord const& hidingSpot) :
	m_owner(owner),
	m_targetStepGenerator(owner)
{
	this->addPendingHidingSpot(hidingSpot);
}

SeekMovementGenerator::~SeekMovementGenerator()
{
	m_owner = nullptr;
}


void SeekMovementGenerator::finish()
{
	if (m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		m_owner->getMoveSpline()->stop();
		m_owner->clearUnitState(UNIT_STATE_MOVING);
	}

	m_pendingHidingSpotLookupTable.clear();
	m_pendingHidingSpotList.clear();
	m_owner->clearUnitState(UNIT_STATE_SEEKING);
	if(m_owner->getAI())
		m_owner->getAI()->clearAIAction(AI_ACTION_TYPE_SEEK);
}

void SeekMovementGenerator::sendMoveStart()
{
	MovementInfo movement = m_owner->getData()->getMovementInfo();
	m_owner->sendMoveOpcodeToNearbyPlayers(MSG_MOVE_START, movement);
}

void SeekMovementGenerator::sendMoveStop()
{
	MovementInfo movement = m_owner->getData()->getMovementInfo();
	m_owner->sendMoveOpcodeToNearbyPlayers(MSG_MOVE_STOP, movement);
}


bool SeekMovementGenerator::update(NSTime diff)
{
	if (!m_owner->isAlive())
		return false;

	if (!m_owner->getMoveSpline()->isFinished())
		return true;

	// 如果没有需要处理的隐藏点则结束移动
	TileCoord currHidingSpot;
	if (!this->selectNextPendingHidingSpot(currHidingSpot))
	{
		this->moveStop();
		return false;
	}
	else
	{
		TileCoord currCoord(m_owner->getData()->getMapData()->getMapSize(), m_owner->getData()->getPosition());
		Point currHidingSpotPos = currHidingSpot.computePosition(m_owner->getData()->getMapData()->getMapSize());
		// 到达隐藏点附近
		if (m_owner->isWithinDist(currHidingSpotPos, DISCOVER_CONCEALED_UNIT_DISTANCE))
		{
			this->removePendingHidingSpot(currHidingSpot);
			m_owner->markHidingSpotChecked(currHidingSpot);

			this->addAdjacentHidingSpots(currHidingSpot);
		}
		// 去隐藏点
		else
		{
			TileCoord nextStep;
			bool moving = m_targetStepGenerator.step(nextStep, currCoord, currHidingSpot);
			if (moving)
			{
				this->moveStart();
				m_owner->getMoveSpline()->moveByStep(nextStep);
			}
		}
	}

	return true;
}

void SeekMovementGenerator::moveStart()
{
	if (!m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStart();
		m_owner->addUnitState(UNIT_STATE_MOVING);
	}
}

void SeekMovementGenerator::moveStop()
{
	if (m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		m_owner->clearUnitState(UNIT_STATE_MOVING);
	}
}

void SeekMovementGenerator::addAdjacentHidingSpots(TileCoord const& currCoord)
{
	this->addPendingHidingSpotIfNeeded(OFFSET_TOP(currCoord));
	this->addPendingHidingSpotIfNeeded(OFFSET_BOTTOM(currCoord));
	this->addPendingHidingSpotIfNeeded(OFFSET_LEFT(currCoord));
	this->addPendingHidingSpotIfNeeded(OFFSET_RIGHT(currCoord));
	this->addPendingHidingSpotIfNeeded(OFFSET_LEFT_TOP(currCoord));
	this->addPendingHidingSpotIfNeeded(OFFSET_RIGHT_TOP(currCoord));
	this->addPendingHidingSpotIfNeeded(OFFSET_LEFT_BOTTOM(currCoord));
	this->addPendingHidingSpotIfNeeded(OFFSET_RIGHT_BOTTOM(currCoord));
}

void SeekMovementGenerator::addPendingHidingSpotIfNeeded(TileCoord const& hidingSpot)
{
	MapData* mapData = m_owner->getData()->getMapData();
	if (mapData->isConcealable(hidingSpot) 
		&& !m_owner->isHidingSpotChecked(hidingSpot) 
		&& m_owner->getMap()->isSafeDistanceMaintained(hidingSpot))
	{
		this->addPendingHidingSpot(hidingSpot);
	}
}

void SeekMovementGenerator::updatePendingHidingSpots()
{
	for (auto it = m_pendingHidingSpotList.begin(); it != m_pendingHidingSpotList.end();)
	{
		TileCoord const& tileCoord = *it;
		int32 tileIndex = m_owner->getData()->getMapData()->getTileIndex(tileCoord);
		if (!m_owner->getMap()->isSafeDistanceMaintained(tileCoord))
		{
			it = m_pendingHidingSpotList.erase(it);
			m_pendingHidingSpotLookupTable.erase(tileIndex);
		}

		else
			++it;
	}
}

bool SeekMovementGenerator::selectNextPendingHidingSpot(TileCoord& hidingSpot)
{
	this->updatePendingHidingSpots();
	auto it = m_pendingHidingSpotList.begin();
	if (it != m_pendingHidingSpotList.end())
	{
		hidingSpot = *it;
		return true;
	}

	return false;
}

void SeekMovementGenerator::addPendingHidingSpot(TileCoord const& hidingSpot)
{
	int32 tileIndex = m_owner->getData()->getMapData()->getTileIndex(hidingSpot);
	auto it = m_pendingHidingSpotLookupTable.find(tileIndex);
	if (it == m_pendingHidingSpotLookupTable.end())
	{
		auto it = m_pendingHidingSpotList.emplace(m_pendingHidingSpotList.end(), hidingSpot);
		NS_ASSERT(it != m_pendingHidingSpotList.end());
		m_pendingHidingSpotLookupTable.emplace(tileIndex, it);
	}
}

bool SeekMovementGenerator::isPendingHidingSpot(TileCoord const& hidingSpot) const
{
	int32 tileIndex = m_owner->getData()->getMapData()->getTileIndex(hidingSpot);
	return m_pendingHidingSpotLookupTable.find(tileIndex) != m_pendingHidingSpotLookupTable.end();
}

void SeekMovementGenerator::removePendingHidingSpot(TileCoord const& hidingSpot)
{
	int32 tileIndex = m_owner->getData()->getMapData()->getTileIndex(hidingSpot);
	auto it = m_pendingHidingSpotLookupTable.find(tileIndex);
	if (it != m_pendingHidingSpotLookupTable.end())
	{
		m_pendingHidingSpotList.erase((*it).second);
		m_pendingHidingSpotLookupTable.erase(it);
	}

}
