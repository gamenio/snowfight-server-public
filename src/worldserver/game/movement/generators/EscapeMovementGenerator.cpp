#include "EscapeMovementGenerator.h"

#include "game/behaviors/Robot.h"
#include "game/utils/MathTools.h"
#include "game/movement/MovementUtil.h"
#include "logging/Log.h"

#define GET_RID_OF_TARGET_DISTANCE				(float)(ABANDON_ATTACK_DISTANCE + TILE_WIDTH)			// 摆脱目标的距离

EscapeMovementGenerator::EscapeMovementGenerator(Robot* owner, Unit* target) :
	TargetMovementGenerator(owner, target),
	m_owner(owner),
	m_jpsPlus(nullptr),
	m_closestToEscapeCoord(TileCoord::INVALID),
	m_isGoingToSafety(false),
	m_targetStepGenerator(owner),
	m_randomStepGenerator(owner)
{
	m_jpsPlus = m_owner->getMap()->getJPSPlus();
}

EscapeMovementGenerator::~EscapeMovementGenerator()
{
	m_jpsPlus = nullptr;
	m_owner = nullptr;
}

bool EscapeMovementGenerator::update(NSTime diff)
{
	if (!m_target.isValid() || !m_target->isInWorld())
		return false;

	if (!m_owner->isAlive())
		return false;

	if (!m_owner->isInCombat())
		return false;

	if (!m_owner->getMoveSpline()->isFinished())
		return true;

	MapData* mapData = m_owner->getMap()->getMapData();
	Point const& currPos = m_owner->getData()->getPosition();
	Point const& targetPos = m_target->getData()->getPosition();
	TileCoord currCoord(mapData->getMapSize(), currPos);

	if (m_isGoingToSafety && m_path.empty())
		m_isGoingToSafety = false;

	if (!m_isGoingToSafety)
	{
		if (m_owner->getMap()->getCurrentSafeZoneRadius() > 0
			&& !m_owner->getMap()->isWithinSafeZone(currCoord))
		{
			m_closestToEscapeCoord = TileCoord::INVALID;
			m_path.clear(); 
			TileCoord patrolPoint;
			m_owner->getMap()->randomPatrolPoint(currCoord, patrolPoint);
			m_isGoingToSafety = m_jpsPlus->getPath(currCoord, patrolPoint, m_path);
		}
	}

	TileCoord toCoord;
	if (m_path.empty())
	{
		bool isCrossing = false;
		bool isDistancing = false;
		ExplorArea const& area = m_owner->getGoalExplorArea();
		if (area != ExplorArea::INVALID)
		{
			if (area == m_owner->getCurrentWaypointExplorArea())
			{
				TileCoord newCoord = this->calcTileCoordToGetRidOfTarget(m_target.getTarget(), area.originInTile);
				bool found = m_jpsPlus->getPath(currCoord, newCoord, m_path);
				if (found)
					m_closestToEscapeCoord = TileCoord::INVALID;
				else
					isCrossing = true;
			}
			else
			{
				// 应该排除原点在追击范围内的探索区域，因为前往原点位置无法改变该区域状态
				Point pos = area.originInTile.computePosition(mapData->getMapSize());
				if (m_target->isWithinDist(pos, ABANDON_ATTACK_DISTANCE))
				{
					m_owner->addExcludedExplorArea(area);
					m_owner->setGoalExplorArea(ExplorArea::INVALID);
					isDistancing = true;
				}
				else
				{
					bool found = m_jpsPlus->getPath(currCoord, area.originInTile, m_path);
					if (found)
						m_closestToEscapeCoord = TileCoord::INVALID;
					else
						isDistancing = true;
				}
			}
		}
		else
			isDistancing = true;

		if (isDistancing)
		{
			toCoord = this->calcTileCoordToGetRidOfTarget(m_target.getTarget());
			if (toCoord == currCoord)
				isCrossing = true;
		}

		if (isCrossing)
		{
			TileCoord newCoord = this->calcTileCoordToGetRidOfTarget(m_target.getTarget(), true);
			bool found = m_jpsPlus->getPath(currCoord, newCoord, m_path);
			if (found)
				m_closestToEscapeCoord = TileCoord::INVALID;
		}
	}

	auto it = m_path.begin();
	if (it != m_path.end())
	{
		TileCoord const& node = *it;
		if (node == currCoord)
		{
			m_closestToEscapeCoord = TileCoord::INVALID;
			m_path.erase(it);
		}
		else
		{
			// 当前往安全位置时不移除目标节点（最后一个节点）
			if (!m_isGoingToSafety || node != m_path.back())
			{
				// 如果节点在目标与我所在位置范围内（不超出攻击范围）则将其移除
				float minR = std::min(m_target->getData()->getAttackRange(), targetPos.getDistance(currPos));
				Point pos = node.computePosition(mapData->getMapSize());
				float d = pos.getDistance(targetPos);
				if (d <= minR)
				{
					m_closestToEscapeCoord = TileCoord::INVALID;
					m_path.erase(it);
				}
			}
		}
	}

	if (!m_path.empty())
	{
		TileCoord const& escapeCoord = m_path.front();
		toCoord = escapeCoord;
		TileCoord newCoord;
		if (MovementUtil::calcTileCoordToBypassTarget(m_owner, m_target.getTarget(), escapeCoord, newCoord))
		{
			float cond = true;
			if (m_closestToEscapeCoord != TileCoord::INVALID)
			{
				float d1 = newCoord.getDistance(toCoord);
				float d2 = m_closestToEscapeCoord.getDistance(toCoord);
				cond = d1 < d2;
			}
			if (cond)
			{
				toCoord = newCoord;
				m_closestToEscapeCoord = newCoord;
			}
		}
	}

	if (!toCoord.isZero())
	{
		bool moving = false;
		TileCoord nextStep;
		if (toCoord == currCoord)
			moving = m_randomStepGenerator.step(nextStep);
		else
			moving = m_targetStepGenerator.step(nextStep, currCoord, toCoord);
		if (moving)
		{
			this->moveStart();
			m_owner->getMoveSpline()->moveByStep(nextStep);
		}
	}

	return true;
}

void EscapeMovementGenerator::moveStart()
{
	if (!m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStart();
		m_owner->addUnitState(UNIT_STATE_MOVING);
	}
}

void EscapeMovementGenerator::moveStop()
{
	if (m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		m_owner->clearUnitState(UNIT_STATE_MOVING);
	}
}

void EscapeMovementGenerator::finish()
{
	if (m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		m_owner->getMoveSpline()->stop();
		m_owner->clearUnitState(UNIT_STATE_MOVING);
	}
}

TileCoord EscapeMovementGenerator::calcTileCoordToGetRidOfTarget(Unit* target, bool isReverse)
{
	MapData* mapData = m_owner->getMap()->getMapData();
	Point const& targetPos = target->getData()->getPosition();
	Point const& currPos = m_owner->getData()->getPosition();

	float maxDist = std::max(GET_RID_OF_TARGET_DISTANCE, targetPos.getDistance(currPos));
	Point point = MathTools::findPointAlongLine(targetPos, currPos, (float)(isReverse ? (-maxDist) : maxDist));
	TileCoord result(mapData->getMapSize(), point);
	if (!mapData->isValidTileCoord(result) || mapData->isWall(result) || m_owner->getMap()->isTileClosed(result))
	{
		std::vector<TileCoord> foundCoordList;
		m_owner->getMap()->findNearestOpenTileList(result, foundCoordList);
		NS_ASSERT(!foundCoordList.empty());
		result = foundCoordList.front();
	}
	return result;
}

TileCoord EscapeMovementGenerator::calcTileCoordToGetRidOfTarget(Unit* target, TileCoord const& goalCoord)
{
	MapData* mapData = m_owner->getMap()->getMapData();
	Point const& targetPos = target->getData()->getPosition();
	Point const& currPos = m_owner->getData()->getPosition();
	Point goalPos = goalCoord.computePosition(mapData->getMapSize());

	TileCoord result;
	std::vector<Point> points;
	MathTools::findIntersectionsLineCircle(currPos, goalPos, targetPos, GET_RID_OF_TARGET_DISTANCE, points);
	if (!points.empty())
	{
		Point bestPoint = points[0];
		if (points.size() == 2)
		{
			bool inside = MathTools::isPointInsideCircle(targetPos, GET_RID_OF_TARGET_DISTANCE, goalPos);
			if (!inside)
			{
				float d1 = points[0].getDistance(goalPos);
				float d2 = points[1].getDistance(goalPos);
				if (d1 < d2)
					bestPoint = points[0];
				else
					bestPoint = points[1];
			}
			else
			{
				float r1 = MathTools::computeAngleInRadians(points[0], goalPos);
				float r2 = MathTools::computeAngleInRadians(points[1], goalPos);
				float r3 = MathTools::computeAngleInRadians(currPos, goalPos);
				if (std::abs(r2 - r3) < std::abs(r1 - r3))
					bestPoint = points[0];
				else
					bestPoint = points[1];
			}
		}
		result = TileCoord(mapData->getMapSize(), bestPoint);
		if (!mapData->isValidTileCoord(result) || mapData->isWall(result) || m_owner->getMap()->isTileClosed(result))
		{
			std::vector<TileCoord> foundCoordList;
			m_owner->getMap()->findNearestOpenTileList(result, foundCoordList);
			NS_ASSERT(!foundCoordList.empty());
			result = foundCoordList.front();
		}
	}
	else
		result = goalCoord;
	 
	return result;
}