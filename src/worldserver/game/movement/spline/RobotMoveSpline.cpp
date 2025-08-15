#include "RobotMoveSpline.h"

#include "utilities/TimeUtil.h"
#include "game/server/protocol/Opcode.h"
#include "game/behaviors/Robot.h"
#include "game/utils/MathTools.h"
#include "game/server/WorldSocket.h"
#include "game/grids/ObjectSearcher.h"
#include "game/behaviors/Player.h"
#include "game/behaviors/UnitLocator.h"
#include "MoveSegment.h"

RobotMoveSpline::RobotMoveSpline(Robot* owner):
	m_owner(owner),
	m_currSegment(nullptr),
	m_realMoveSpeed(0)
{
}


RobotMoveSpline::~RobotMoveSpline()
{
	m_owner = nullptr;

	if (m_currSegment)
	{
		delete m_currSegment;
		m_currSegment = nullptr;
	}
}


void RobotMoveSpline::update(NSTime diff)
{
	if (!m_currSegment)
		return;

	// Check if the moving speed has changed
	if (m_owner->getData()->hasUpdatedField(SUNIT_FIELD_MOVE_SPEED) && m_realMoveSpeed > 0)
	{
		int32 moveSpeed = m_owner->getData()->getMoveSpeed();
		float speedScale = moveSpeed / (float)m_realMoveSpeed;
		m_currSegment->setSpeedScale(speedScale);
	}

	m_currSegment->step(diff);
	if (m_currSegment->isDone())
	{
		m_owner->getUnitHostileRefManager()->updateThreat(UNITTHREAT_DISTANCE);
		m_owner->getUnitThreatManager()->updateAllThreats(UNITTHREAT_DISTANCE);
		this->sendRelocateLocator(m_owner->getData()->getPosition());

		delete m_currSegment;
		m_currSegment = nullptr;

		this->popStepAndWalk();
	}
}


void RobotMoveSpline::popStepAndWalk()
{
	// Check if there are still path steps to go
	if (m_path.empty())
	{
		m_owner->getData()->clearMovementFlag(MOVEMENT_FLAG_WALKING);

		m_isFinished = true;
		return;
	}

	MapData const* mapData = m_owner->getMap()->getMapData();
	Point startPos = m_owner->getData()->getPosition();
	Point endPos = *m_path.begin();
	TileCoord fromCoord(mapData->getMapSize(), startPos);
	TileCoord toCoord(mapData->getMapSize(), endPos);

	m_owner->getData()->setOrientation(MathTools::computeAngleInRadians(startPos, endPos));

	MovementInfo movement = m_owner->getData()->getMovementInfo();
	movement.position = endPos;
	m_owner->getData()->markMoveSegment(movement);

	this->sendMoveSync(endPos);

	m_realMoveSpeed = m_owner->getData()->getMoveSpeed();
	float length = endPos.getDistance(startPos);
	NSTime duration = MathTools::computeMovingTimeMs(length, m_realMoveSpeed);
	//NS_LOG_DEBUG("movement.spline.robot", "ROBOT STEP guid: %s start:[%f, %f][%d, %d] end:[%f, %f][%d, %d] duration: %d",
	//	m_owner->getData()->getGuid().toString().c_str(), 
	//	startPos.x, startPos.y, fromCoord.x, fromCoord.y,
	//	endPos.x, endPos.y, toCoord.x, toCoord.y, duration);

	if (m_currSegment)
	{
		delete m_currSegment;
		m_currSegment = nullptr;
	}
	m_currSegment = new MoveSegment(m_owner, duration, endPos);

	m_path.erase(m_path.begin());
}

void RobotMoveSpline::sendMoveSync(Point const& pos)
{
	MovementInfo movement = m_owner->getData()->getMovementInfo();
	movement.position = pos;
	m_owner->sendMoveOpcodeToNearbyPlayers(MSG_MOVE_SYNC, movement);
}

void RobotMoveSpline::sendRelocateLocator(Point const& pos)
{
	if (!m_owner->getLocator())
		return;

	LocationInfo location = m_owner->getLocator()->getData()->getLocationInfo();
	location.position = pos;
	m_owner->sendLocationInfoToNearbyPlayers(location);
}

void RobotMoveSpline::moveByStep(TileCoord const& step)
{
	MapData const* mapData = m_owner->getMap()->getMapData();
	Point const& pos = step.computePosition(mapData->getMapSize());
	this->moveByPosition(pos);
}

void RobotMoveSpline::moveByPosition(Point const& position)
{
	m_isFinished = false;
	m_path.push_back(position);

	m_owner->getData()->addMovementFlag(MOVEMENT_FLAG_WALKING);
	this->popStepAndWalk();
}

void RobotMoveSpline::stop()
{
	if (m_isFinished)
		return;

	if (m_currSegment)
	{
		delete m_currSegment;
		m_currSegment = nullptr;
	}

	m_owner->getData()->clearMoveSegment();
	m_owner->getData()->clearMovementFlag(MOVEMENT_FLAG_WALKING);

	m_realMoveSpeed = 0;
	m_path.clear();
	m_isFinished = true;
}