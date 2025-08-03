#include "PointMovementGenerator.h"

#include "logging/Log.h"
#include "game/server/protocol/Opcode.h"
#include "game/movement/MovementUtil.h"

PointMovementGenerator::PointMovementGenerator(Robot* owner, TileCoord const& point, bool isBypassEnemy) :
	m_owner(owner),
	m_point(point),
	m_isBypassEnemy(isBypassEnemy),
	m_closestToPointCoord(TileCoord::INVALID),
	m_targetStepGenerator(owner)
{
	this->initialize();
}

PointMovementGenerator::~PointMovementGenerator()
{
	m_owner = nullptr;
}

bool PointMovementGenerator::update(NSTime diff)
{
	if (!m_owner->isAlive())
		return false;

	if (!m_owner->getMoveSpline()->isFinished())
		return true;

	TileCoord currCoord(m_owner->getData()->getMapData()->getMapSize(), m_owner->getData()->getPosition());
	// 到达目标点
	if (currCoord == m_realPoint)
	{
		this->moveStop();
		return false;
	}
	// 前往目标点
	else
	{
		TileCoord toCoord = m_realPoint;
		if (m_isBypassEnemy)
		{
			Unit* enemy = m_owner->getVictim();
			if (enemy)
			{
				TileCoord newCoord;
				if (MovementUtil::calcTileCoordToBypassTarget(m_owner, enemy, m_realPoint, newCoord))
				{
					float cond = true;
					if (m_closestToPointCoord != TileCoord::INVALID)
					{
						float d1 = newCoord.getDistance(toCoord);
						float d2 = m_closestToPointCoord.getDistance(toCoord);
						cond = d1 < d2;
					}
					if (cond)
					{
						toCoord = newCoord;
						m_closestToPointCoord = newCoord;
					}
				}
			}
		}

		TileCoord nextStep;
		bool moving = m_targetStepGenerator.step(nextStep, currCoord, toCoord);
		if (moving)
		{
			this->moveStart();
			m_owner->getMoveSpline()->moveByStep(nextStep);
		}
	}

	return true;
}

void PointMovementGenerator::finish()
{
	if (m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		m_owner->getMoveSpline()->stop();
		m_owner->clearUnitState(UNIT_STATE_MOVING);
	}
}

void PointMovementGenerator::initialize()
{
	MapData* mapData = m_owner->getMap()->getMapData();
	TileCoord currCoord(mapData->getMapSize(), m_owner->getData()->getPosition());
	// 找到距离point最近的开放位置
	if (!mapData->isValidTileCoord(m_point) || mapData->isWall(m_point) || m_owner->getMap()->isTileClosed(m_point))
		m_owner->getMap()->findNearestOpenTile(m_point, currCoord, m_realPoint);
	else
		m_realPoint = m_point;
}

void PointMovementGenerator::sendMoveStart()
{
	MovementInfo movement = m_owner->getData()->getMovementInfo();
	m_owner->sendMoveOpcodeToNearbyPlayers(MSG_MOVE_START, movement);
}

void PointMovementGenerator::sendMoveStop()
{
	MovementInfo movement = m_owner->getData()->getMovementInfo();
	m_owner->sendMoveOpcodeToNearbyPlayers(MSG_MOVE_STOP, movement);
}


void PointMovementGenerator::moveStart()
{
	if (!m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStart();
		m_owner->addUnitState(UNIT_STATE_MOVING);
	}
}

void PointMovementGenerator::moveStop()
{
	if (m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		m_owner->clearUnitState(UNIT_STATE_MOVING);
	}
}