#include "SmartChaseMovementGenerator.h"

#include "logging/Log.h"
#include "game/server/protocol/Opcode.h"
#include "game/behaviors/Projectile.h"
#include "game/movement/spline/MoveSpline.h"
#include "game/world/ObjectMgr.h"
#include "MovementGenerator.h"


#define DODGE_DISTANCE								64 			// The distance moved for each dodge

// Dodge duration time range. Unit: milliseconds
template<> int32 SmartChaseMovementGenerator<Unit>::DODGE_DURATION_MIN = 3000;
template<> int32 SmartChaseMovementGenerator<Unit>::DODGE_DURATION_MAX = 5000;
template<> int32 SmartChaseMovementGenerator<ItemBox>::DODGE_DURATION_MIN = 500;
template<> int32 SmartChaseMovementGenerator<ItemBox>::DODGE_DURATION_MAX = 2000;

template<typename TARGET>
SmartChaseMovementGenerator<TARGET>::SmartChaseMovementGenerator(Robot* owner, TARGET* target):
	TargetMovementGenerator<TARGET>(owner, target),
	m_targetStepGenerator(owner),
	m_randomStepGenerator(owner),
	m_isChasing(false),
	m_reactionTimer(true),
	m_nextDodgeDirection(DODGE_NONE),
	m_currDodgeDirection(DODGE_NONE),
	m_isLocked(false)
{
	this->initialize();
}

template<typename TARGET>
SmartChaseMovementGenerator<TARGET>::~SmartChaseMovementGenerator()
{
}


template<typename TARGET>
void SmartChaseMovementGenerator<TARGET>::finish()
{
	if (this->m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		this->m_owner->getMoveSpline()->stop();
		this->m_owner->clearUnitState(UNIT_STATE_MOVING);
	}
}

template<typename TARGET>
bool SmartChaseMovementGenerator<TARGET>::update(NSTime diff)
{
	if (!this->m_target.isValid() || !this->m_target->isInWorld())
		return false;

	if (!this->m_owner->isAlive())
		return false;

	NS_ASSERT(this->m_owner->isInCombat() || this->m_owner->isInUnlock());

	if (m_currDodgeDirection != DODGE_NONE)
	{
		m_dodgeTimer.update(diff);
	}

	Projectile* proj = this->m_owner->selectProjectileToDodge();
	if (proj)
	{
		m_nextDodgeDirection = this->calcDodgeDirection(proj);
		RobotProficiency const* proficiency = sObjectMgr->getRobotProficiency(this->m_owner->getData()->getProficiencyLevel());
		NS_ASSERT(proficiency);
		if (!rollChance(proficiency->effectiveDodgeChance))
			m_nextDodgeDirection = m_nextDodgeDirection == DODGE_CLOCKWISE ? DODGE_ANTICLOCKWISE : DODGE_CLOCKWISE;
	}
	else
		m_nextDodgeDirection = DODGE_NONE;

	bool isDodging = false;
	m_reactionTimer.update(diff);
	if (m_reactionTimer.passed())
	{
		if (m_currDodgeDirection != m_nextDodgeDirection)
		{
			if (m_nextDodgeDirection != DODGE_NONE)
			{
				if (this->m_owner->hasUnitState(UNIT_STATE_MOVING))
				{
					this->sendMoveStop();
					this->m_owner->getMoveSpline()->stop();
					this->m_owner->clearUnitState(UNIT_STATE_MOVING);
				}
				m_currDodgeDirection = m_nextDodgeDirection;
				isDodging = true;
				this->updateDodgeDuration();
			}
		}
		m_reactionTimer.reset();
	}

	if (!this->m_owner->getMoveSpline()->isFinished())
		return true;

	MapData* mapData = this->m_owner->getMap()->getMapData();
	Point const& currPos = this->m_owner->getData()->getPosition();
	Point const&  targetPos = this->m_target->getData()->getPosition();
	TileCoord currCoord(mapData->getMapSize(), currPos);
	TileCoord targetCoord(mapData->getMapSize(), targetPos);

	TileCoord toCoord;
	// The target is within effective attack range
	if (this->m_owner->isWithinEffectiveRange(this->m_target.getTarget()))
	{
		// Dodge the danger zone
		bool isKeepDistance = true;
		if (this->m_owner->getMap()->getCurrentSafeZoneRadius() > 0 &&
			!this->m_owner->getMap()->isSafeDistanceMaintained(currCoord))
		{
			Point refPoint = this->m_owner->getMap()->getSafeZoneCenter().computePosition(mapData->getMapSize());
			Point goalPos = MathTools::findPointAlongLine(targetPos, refPoint, this->m_owner->getData()->getAttackRange());
			if (this->m_owner->getMap()->isSafeDistanceMaintained(targetCoord) || !this->m_owner->getMap()->isWithinSafeZone(currCoord))
			{
				m_currDodgeDirection = this->calcDodgeDirection(this->m_target.getTarget(), goalPos);
				isKeepDistance = false;
				isDodging = true;
				this->updateDodgeDuration();
			}
		}

		if (!isDodging && (m_dodgeTimer.passed() || !m_isLocked))
		{
			// Whether the target is locked
			m_isLocked = this->m_owner->lockingTarget(this->m_target.getTarget());
			if (m_isLocked)
			{
				if (m_currDodgeDirection == DODGE_NONE)
					m_currDodgeDirection = random(0, 1) == 0 ? DODGE_ANTICLOCKWISE : DODGE_CLOCKWISE;
				else
					m_currDodgeDirection = m_currDodgeDirection == DODGE_CLOCKWISE ? DODGE_ANTICLOCKWISE : DODGE_CLOCKWISE;
				this->updateDodgeDuration();
			}
			else
			{
				m_currDodgeDirection = DODGE_NONE;
				m_dodgeTimer.reset();
			}
		}

		if (m_isLocked)
		{
			NS_ASSERT_DEBUG(m_currDodgeDirection != DODGE_NONE);
			toCoord = this->calcDodgeTileCoordAroundTarget(this->m_target.getTarget(), m_currDodgeDirection, isKeepDistance);
		}
		else
		{
			if (m_currDodgeDirection != DODGE_NONE)
				toCoord = this->calcDodgeTileCoordToTarget(this->m_target.getTarget(), m_currDodgeDirection);
			else
				toCoord = this->getClosestTileCoordToTarget(this->m_target.getTarget());
		}

		m_isChasing = false;
	}
	// Start chasing the target
	else
	{
		if (!isDodging && m_dodgeTimer.passed())
		{
			m_currDodgeDirection = DODGE_NONE;
			m_dodgeTimer.reset();
		}

		if (m_currDodgeDirection != DODGE_NONE)
			toCoord = this->calcDodgeTileCoordToTarget(this->m_target.getTarget(), m_currDodgeDirection);
		else
			toCoord = this->getClosestTileCoordToTarget(this->m_target.getTarget());

		m_isLocked = false;
		m_isChasing = true;
	}

	TileCoord nextStep;
	bool moving;
	if (currCoord == toCoord)
		moving = m_randomStepGenerator.step(nextStep);
	else
		moving = this->m_targetStepGenerator.step(nextStep, currCoord, toCoord);
	if (moving)
	{
		this->moveStart();
		this->m_owner->getMoveSpline()->moveByStep(nextStep);
	}

	return true;
}

template<typename TARGET>
void SmartChaseMovementGenerator<TARGET>::initialize()
{
	RobotProficiency const* proficiency = sObjectMgr->getRobotProficiency(this->m_owner->getData()->getProficiencyLevel());
	NS_ASSERT(proficiency);
	int32 reactionInr = random(proficiency->minDodgeReactionTime, proficiency->maxDodgeReactionTime);
	m_reactionTimer.setInterval(reactionInr);

	m_dodgeTimer.setDuration(DODGE_DURATION_MIN);
	m_dodgeTimer.setPassed();
}

template<typename TARGET>
void SmartChaseMovementGenerator<TARGET>::moveStart()
{
	if (!this->m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStart();
		this->m_owner->addUnitState(UNIT_STATE_MOVING);
	}
}

template<typename TARGET>
void SmartChaseMovementGenerator<TARGET>::moveStop()
{
	if (this->m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		this->m_owner->setFacingToObject(this->m_target.getTarget());
		this->m_owner->clearUnitState(UNIT_STATE_MOVING);
	}
}

template<typename TARGET>
TileCoord SmartChaseMovementGenerator<TARGET>::calcDodgeTileCoordAroundTarget(TARGET* target, DodgeDirection dir, bool isKeepDistance)
{
	Point startPos = target->getData()->getPosition();
	Point endPos = this->m_owner->getData()->getPosition();
	float dx = endPos.x - startPos.x;
	float dy = endPos.y - startPos.y;
	float r = this->m_owner->calcOptimalDistanceToDodgeTarget(target);
	if (!isKeepDistance)
	{
		float d = startPos.getDistance(endPos);
		r = std::min(r, d);
	}
	float theta = std::atan2(dy, dx);
	float rad = (float)DODGE_DISTANCE / r; // Find the radian by radius and arc length
	float angle = theta + (dir == DODGE_CLOCKWISE ? -rad : rad);

	Point offset;
	offset.x = std::cos(angle) * r;
	offset.y = std::sin(angle) * r;
	Point newPos = startPos + offset;
	TileCoord result(this->m_owner->getData()->getMapData()->getMapSize(), newPos);

	MapData* mapData = this->m_owner->getMap()->getMapData();
	if(!mapData->isValidTileCoord(result) || mapData->isWall(result) || this->m_owner->getMap()->isTileClosed(result))
		this->m_owner->getMap()->findNearestOpenTile(result, result);

	return result;
}

template<typename TARGET>
TileCoord SmartChaseMovementGenerator<TARGET>::calcDodgeTileCoordToTarget(TARGET* target, DodgeDirection dir)
{
	Point newPos = target->getData()->getPosition();
	Point tp1, tp2;
	bool ret = MathTools::findTangentPointsInCircle(target->getData()->getPosition(), this->m_owner->getData()->getPosition(), DODGE_DISTANCE, tp1, tp2);
	if (ret)
	{
		if (dir == DODGE_CLOCKWISE)
			newPos = tp1;
		else
			newPos = tp2;
	}

	TileCoord result(this->m_owner->getData()->getMapData()->getMapSize(), newPos);
	MapData* mapData = this->m_owner->getMap()->getMapData();
	if (!mapData->isValidTileCoord(result) || mapData->isWall(result) || this->m_owner->getMap()->isTileClosed(result))
		this->m_owner->getMap()->findNearestOpenTile(result, result);

	return result;
}

template<typename TARGET>
TileCoord SmartChaseMovementGenerator<TARGET>::getClosestTileCoordToTarget(TARGET* target)
{
	MapData* mapData = this->m_owner->getMap()->getMapData();
	TileCoord currCoord(mapData->getMapSize(), this->m_owner->getData()->getPosition());
	TileCoord targetCoord(mapData->getMapSize(), target->getData()->getPosition());

	TileCoord result;
	if (!mapData->isValidTileCoord(targetCoord) || mapData->isWall(targetCoord) || this->m_owner->getMap()->isTileClosed(targetCoord))
		this->m_owner->getMap()->findNearestOpenTile(targetCoord, currCoord, result);
	else
		result = targetCoord;

	return result;
}

template<typename TARGET>
DodgeDirection SmartChaseMovementGenerator<TARGET>::calcDodgeDirection(Projectile* proj)
{
	DodgeDirection dir;
	Point p1 = proj->getData()->getLauncherOrigin();
	Point p2 = proj->getData()->getTrajectory().startPosition + proj->getData()->getTrajectory().endPosition;
	Point p = this->m_owner->getData()->getPosition();
	float d = MathTools::findPointOnWhichSideOfLine(p, p1, p2);
	if (d > 0)
		dir = DODGE_CLOCKWISE;
	else
		dir = DODGE_ANTICLOCKWISE;

	return dir;
}

template<typename TARGET>
DodgeDirection SmartChaseMovementGenerator<TARGET>::calcDodgeDirection(TARGET* target, Point const& goalPos)
{
	MapData* mapData = this->m_owner->getMap()->getMapData();
	Point const& currPos = this->m_owner->getData()->getPosition();
	Point const& targetPos = target->getData()->getPosition();

	DodgeDirection dir;
	float a1 = MathTools::radians2Degrees(MathTools::computeAngleInRadians(targetPos, goalPos));
	if (a1 < 0)
		a1 += 360.f;
	float a2 = MathTools::radians2Degrees(MathTools::computeAngleInRadians(targetPos, currPos));
	if (a2 < 0)
		a2 += 360.f;

	float d1, d2;
	if (a2 > a1)
	{
		d1 = a2 - a1;
		d2 = 360.f - a2 + a1;
	}
	else
	{
		d1 = 360.f - a1 + a2;
		d2 = a1 - a2;
	}

	if (d1 < d2)
		dir = DODGE_CLOCKWISE;
	else
		dir = DODGE_ANTICLOCKWISE;

	return dir;
}

template<typename TARGET>
void SmartChaseMovementGenerator<TARGET>::updateDodgeDuration()
{
	int32 dur = random(DODGE_DURATION_MIN, DODGE_DURATION_MAX);
	m_dodgeTimer.setDuration(dur);
}

template class SmartChaseMovementGenerator<Unit>;
template class SmartChaseMovementGenerator<ItemBox>;
