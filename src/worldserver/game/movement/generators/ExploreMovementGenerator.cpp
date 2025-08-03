#include "ExploreMovementGenerator.h"

#include "logging/Log.h"
#include "utilities/TimeUtil.h"
#include "game/behaviors/Robot.h"


#define OFFSET_TOP(area)				ExplorArea(area.x, area.y + 1)
#define OFFSET_BOTTOM(area)				ExplorArea(area.x, area.y - 1)
#define OFFSET_LEFT(area)				ExplorArea(area.x - 1, area.y)
#define OFFSET_RIGHT(area)				ExplorArea(area.x + 1, area.y)
#define OFFSET_LEFT_TOP(area)			ExplorArea(area.x - 1, area.y + 1)
#define OFFSET_LEFT_BOTTOM(area)		ExplorArea(area.x - 1, area.y - 1)
#define OFFSET_RIGHT_TOP(area)			ExplorArea(area.x + 1, area.y + 1)
#define OFFSET_RIGHT_BOTTOM(area)		ExplorArea(area.x + 1, area.y - 1)

ExploreMovementGenerator::ExploreMovementGenerator(Robot* owner) :
	m_owner(owner),
	m_stepGenerator(owner),
	m_explorState(EXPLOR_STATE_NONE),
	m_currWaypointExplorArea(ExplorArea::INVALID),
	m_targetWaypoint(TileCoord::INVALID),
	m_goalPatrolPoint(TileCoord::INVALID)
{
	this->initialize();
}

ExploreMovementGenerator::~ExploreMovementGenerator()
{
	NS_LOG_DEBUG("movement.generator.explore", "Destroy ExploreMovementGenerator");
	m_owner = nullptr;
}

bool ExploreMovementGenerator::update(NSTime diff)
{
	if (!m_owner->isAlive())
		return false;

	if (!m_owner->isInExploration())
		return false;

	if (!m_owner->getMoveSpline()->isFinished())
		return true;

	MapData* mapData = m_owner->getMap()->getMapData();
	TileCoord currCoord(mapData->getMapSize(), m_owner->getData()->getPosition());

	if (m_explorState != EXPLOR_STATE_PATROLLING
		&& (currCoord == m_goalExplorArea.originInTile)
		&& m_owner->getMap()->checkPatrolConditions(currCoord))
	{
		m_owner->setGoalExplorArea(ExplorArea::INVALID);
		m_owner->setCurrentWaypointExplorArea(ExplorArea::INVALID);
		m_owner->resetStepCounter();
		m_owner->setInPatrol(true);

		m_currExplorArea = ExplorArea::INVALID;
		m_goalExplorArea = ExplorArea::INVALID;
		m_currWaypointExplorArea = ExplorArea::INVALID;
		m_targetWaypoint = TileCoord::INVALID;

		m_goalPatrolPoint = currCoord;
		m_explorState = EXPLOR_STATE_PATROLLING;
		NS_LOG_DEBUG("movement.generator.explore", "Start patrolling");

	}

	if (m_explorState == EXPLOR_STATE_PATROLLING)
	{
		if (currCoord == m_goalPatrolPoint)
		{
			m_owner->increaseStepCount();
			m_owner->getMap()->randomPatrolPoint(currCoord, m_goalPatrolPoint);
		}
		this->move(currCoord, m_goalPatrolPoint);
	}
	else
	{
		if (currCoord == m_goalExplorArea.originInTile)
		{
			NS_ASSERT(m_goalExplorArea != ExplorArea::INVALID);
			m_owner->removeUnexploredExplorArea(m_goalExplorArea);
			m_owner->markAreaExplored(m_goalExplorArea);
			m_currExplorArea = m_goalExplorArea;

			this->moveStop();
			m_owner->increaseStepCount();
			m_owner->updateExcludedExplorAreas();

			auto currTime = getHighResolutionTimeMillis();
			m_goalExplorArea = ExplorArea::INVALID;
			m_explorState = this->nextExplorArea(m_currExplorArea, m_goalExplorArea);
			m_owner->setGoalExplorArea(m_goalExplorArea);

			auto elapsed = getHighResolutionTimeMillis() - currTime;
			NS_LOG_DEBUG("movement.generator.explore", "Exploring time in %f ms", elapsed);
		}

		if (m_explorState != EXPLOR_STATE_NO_AREAS)
		{
			switch (m_explorState)
			{
			case EXPLOR_STATE_EXPLORING:
			case EXPLOR_STATE_GOTO_LINKED_WAYPOINT:
				this->move(currCoord, m_goalExplorArea.originInTile);
				break;
			case EXPLOR_STATE_GOTO_UNEXPLORED:
			case EXPLOR_STATE_GOTO_WAYPOINT:
			{
				ExplorArea newArea;
				if (this->moveAndExploreNewArea(m_currExplorArea, currCoord, m_goalExplorArea, newArea))
				{
					if (newArea != ExplorArea::INVALID)
					{
						m_currWaypointExplorArea = ExplorArea::INVALID;
						m_owner->setCurrentWaypointExplorArea(ExplorArea::INVALID);
						m_owner->setGoalExplorArea(newArea);
						if (newArea != m_goalExplorArea || newArea.originInTile != m_goalExplorArea.originInTile)
							NS_LOG_DEBUG("movement.generator.explore", "Change explor route: %d,%d -> %d,%d (originInTile: %d,%d)", m_currExplorArea.x, m_currExplorArea.y,
								newArea.x, newArea.y, newArea.originInTile.x, newArea.originInTile.y);

						m_goalExplorArea = newArea;
						m_explorState = EXPLOR_STATE_EXPLORING;
					}
				}
				break;
			}
			default:
				break;
			}
		}
		else
		{
			ExplorArea safeArea;
			TileCoord nextStep;
			if (this->moveAndExploreSafeArea(m_currExplorArea, currCoord, nextStep, safeArea))
			{
				if (safeArea != ExplorArea::INVALID)
				{
					NS_LOG_DEBUG("movement.generator.explore", "Goto safe area: %d,%d -> %d,%d", m_currExplorArea.x, m_currExplorArea.y, safeArea.x, safeArea.y);

					m_owner->setGoalExplorArea(safeArea);
					m_goalExplorArea = safeArea;
					m_explorState = EXPLOR_STATE_EXPLORING;
				}
				else
				{
					NS_ASSERT(m_goalExplorArea == ExplorArea::INVALID);
					TileCoord const& safeZoneCenter = m_owner->getMap()->getSafeZoneCenter();
					if (nextStep == safeZoneCenter)
					{
						NS_LOG_DEBUG("movement.generator.explore", "Goto safezone center (tile): %d,%d -> %d,%d", m_currExplorArea.originInTile.x, m_currExplorArea.originInTile.y, safeZoneCenter.x, safeZoneCenter.y);
						m_goalExplorArea.originInTile = safeZoneCenter;
					}
				}
			}
		}
	}

	return true;
}

void ExploreMovementGenerator::moveStart()
{
	if (!m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStart();
		m_owner->addUnitState(UNIT_STATE_MOVING);
	}
}

void ExploreMovementGenerator::moveStop()
{
	if (m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		m_owner->clearUnitState(UNIT_STATE_MOVING);
	}
}

ExploreMovementGenerator::ExplorState ExploreMovementGenerator::nextExplorArea(ExplorArea const& currArea, ExplorArea& result)
{
	MapData* mapData = m_owner->getMap()->getMapData();
	if (m_currWaypointExplorArea == currArea)
	{
		NS_ASSERT(m_currWaypointExplorArea.originInTile == currArea.originInTile);
		TileCoord const& sourceWaypoint = m_owner->getSourceWaypoint();
		bool ret = mapData->getLinkedWaypoint(sourceWaypoint, m_targetWaypoint);
		NS_ASSERT(ret);
		NS_LOG_DEBUG("movement.generator.explore", "Start from waypointArea: %d,%d (originInTile: %d,%d) waypoint: %d,%d -> %d,%d", currArea.x, currArea.y, currArea.originInTile.x, currArea.originInTile.y, sourceWaypoint.x, sourceWaypoint.y, m_targetWaypoint.x, m_targetWaypoint.y);

		TileCoord point;
		m_owner->getSafePointFromWaypointExtent(m_targetWaypoint, point);
		Point pos = point.computePosition(mapData->getMapSize());
		ExplorArea area = m_owner->getExplorArea(pos);
		area.originInTile = point;
		result = area;

		NS_LOG_DEBUG("movement.generator.explore", "GOTO_LINKED_WAYPOINT %d,%d -> %d,%d (originInTile: %d,%d)", currArea.x, currArea.y, result.x, result.y, result.originInTile.x, result.originInTile.y);
		m_currWaypointExplorArea = ExplorArea::INVALID;
		m_owner->setCurrentWaypointExplorArea(ExplorArea::INVALID);

		if (m_owner->isAllDistrictsExplored())
		{
			m_owner->resetExploredAreas();
			m_owner->clearCheckedHidingSpots();
			m_owner->increaseWorldExplorCounter();
		}

		return EXPLOR_STATE_GOTO_LINKED_WAYPOINT;
	}

	DistExplorAreaContainer adjacentAreas;
	const uint32 ADJACENT_FILTER = FILTER_TYPE_SAME_DISTRICT | FILTER_TYPE_UNEXPLORED | FILTER_TYPE_NOT_EXCLUDED;
	this->getAdjacentExplorAreas(currArea, adjacentAreas, ADJACENT_FILTER);

	if (m_owner->getMap()->getExplorableDistrictCount() <= 1
		&& adjacentAreas.empty()
		&& mapData->isSameDistrict(currArea.originInTile, m_owner->getMap()->getSafeZoneCenter()))
	{
		m_owner->updateUnexploredExplorAreas(currArea);
		if (!m_owner->hasUnexploredExplorArea(currArea))
		{
			m_owner->resetExploredAreas();
			m_owner->clearCheckedHidingSpots();
			m_owner->increaseWorldExplorCounter();
			this->getAdjacentExplorAreas(currArea, adjacentAreas, ADJACENT_FILTER);
		}
	}

	auto it = adjacentAreas.begin();
	if (it != adjacentAreas.end())
	{
		auto const& aArea = *it;
		result = aArea.second;

		it = std::next(it);
		for (; it != adjacentAreas.end(); ++it)
		{
			auto const& area = (*it).second;
			NS_ASSERT(m_owner->getExplorOrder(area) == 0);
			m_owner->addUnexploredExplorArea(area);
		}
		m_currWaypointExplorArea = ExplorArea::INVALID;
		m_owner->setCurrentWaypointExplorArea(ExplorArea::INVALID);
		NS_LOG_DEBUG("movement.generator.explore", "EXPLORING %d,%d -> %d,%d movedist: %f", currArea.x, currArea.y, result.x, result.y, aArea.first);

		return EXPLOR_STATE_EXPLORING;
	}
	else
	{
		if (m_owner->hasUnexploredExplorArea(currArea))
		{
			ExplorArea unexploredArea;
			if (m_owner->selectNextUnexploredExplorArea(currArea, unexploredArea))
			{
				result = unexploredArea;
				NS_LOG_DEBUG("movement.generator.explore", "GOTO_UNEXPLORED %d,%d -> %d,%d", currArea.x, currArea.y, result.x, result.y);
				return EXPLOR_STATE_GOTO_UNEXPLORED;
			}
		}

		NS_ASSERT(m_currWaypointExplorArea == ExplorArea::INVALID);
		TileCoord sourceWaypoint;
		if (m_owner->selectWaypointExplorArea(currArea, m_currWaypointExplorArea, sourceWaypoint))
		{
			auto const& waypointArea = m_currWaypointExplorArea;
			auto const& originInTile = m_currWaypointExplorArea.originInTile;
			NS_LOG_DEBUG("movement.generator.explore", "Set waypointArea: %d,%d (originInTile: %d,%d) waypoint: %d,%d", waypointArea.x, waypointArea.y, originInTile.x, originInTile.y, sourceWaypoint.x, sourceWaypoint.y);

			m_owner->setSourceWaypoint(sourceWaypoint);
			m_owner->setCurrentWaypointExplorArea(m_currWaypointExplorArea);
			result = waypointArea;
			if (waypointArea == currArea)
				NS_LOG_DEBUG("movement.generator.explore", "Current explor area is waypointArea: %d,%d (originInTile: %d,%d) waypoint: %d,%d", waypointArea.x, waypointArea.y, originInTile.x, originInTile.y, sourceWaypoint.x, sourceWaypoint.y);
			else
				NS_LOG_DEBUG("movement.generator.explore", "GOTO_WAYPOINT %d,%d -> %d,%d (originInTile: %d,%d)", currArea.x, currArea.y, result.x, result.y, result.originInTile.x, result.originInTile.y);

			return EXPLOR_STATE_GOTO_WAYPOINT;
		}
		else
		{
			NS_LOG_DEBUG("movement.generator.explore", "No areas to explore.");
		}
	}

	return EXPLOR_STATE_NO_AREAS;
}

void ExploreMovementGenerator::getAdjacentExplorAreas(ExplorArea const& currArea, DistExplorAreaContainer& result, uint32 filter)
{
	result.clear();

	this->addExplorAreaIfNeeded(currArea, OFFSET_TOP(currArea), result, filter);
	this->addExplorAreaIfNeeded(currArea, OFFSET_BOTTOM(currArea), result, filter);
	this->addExplorAreaIfNeeded(currArea, OFFSET_LEFT(currArea), result, filter);
	this->addExplorAreaIfNeeded(currArea, OFFSET_RIGHT(currArea), result, filter);
	this->addExplorAreaIfNeeded(currArea, OFFSET_LEFT_TOP(currArea), result, filter);
	this->addExplorAreaIfNeeded(currArea, OFFSET_RIGHT_TOP(currArea), result, filter);
	this->addExplorAreaIfNeeded(currArea, OFFSET_LEFT_BOTTOM(currArea), result, filter);
	this->addExplorAreaIfNeeded(currArea, OFFSET_RIGHT_BOTTOM(currArea), result, filter);
}

void ExploreMovementGenerator::addExplorAreaIfNeeded(ExplorArea const& currArea, ExplorArea const& newArea, DistExplorAreaContainer& areaList, uint32 filter)
{
	MapData* mapData = m_owner->getMap()->getMapData();
	if (currArea == newArea)
	{
		float moveDist = 0.f;
		areaList.emplace(moveDist, currArea);
		NS_LOG_DEBUG("movement.generator.explore", "%d,%d originInTile: %d,%d moveDist: %f", newArea.x, newArea.y, currArea.originInTile.x, currArea.originInTile.y, moveDist);
	}
	else
	{
		bool cond = true;
		if ((filter & FILTER_TYPE_UNEXPLORED) != 0)
			cond = m_owner->getExplorOrder(newArea) == 0;
		if (cond && (filter & FILTER_TYPE_NOT_EXCLUDED) != 0)
			cond = !m_owner->isExplorAreaExcluded(newArea);

		TileCoord toCoord;
		if (cond)
		{
			if (this->getSafeTileInExplorArea(newArea, toCoord))
			{
				NS_ASSERT(m_owner->getMap()->isSafeDistanceMaintained(toCoord));
				NS_ASSERT(!m_owner->getMap()->isDangerousDistrict(toCoord));

				if ((filter & FILTER_TYPE_SAME_DISTRICT) != 0)
					cond = mapData->isSameDistrict(currArea.originInTile, toCoord);

				if (cond)
				{
					ExplorArea hitArea = newArea;
					hitArea.originInTile = toCoord;
					Point startPos = currArea.originInTile.computePosition(mapData->getMapSize());
					Point endPos = toCoord.computePosition(mapData->getMapSize());
					float moveDist = startPos.getDistance(endPos);
					areaList.emplace(moveDist, hitArea);
					NS_LOG_DEBUG("movement.generator.explore", "%d,%d originInTile: %d,%d moveDist: %f", hitArea.x, hitArea.y, hitArea.originInTile.x, hitArea.originInTile.y, moveDist);
				}
			}
		}

		//if (cond)
		//	NS_LOG_DEBUG("movement.generator.explore", "Ignored %d,%d originInTile: %d,%d order: %d toCoord: %d,%d currArea.originInTile: %d,%d", newArea.x, newArea.y, newArea.originInTile.x, newArea.originInTile.y, m_owner->getExplorOrder(newArea), toCoord.x, toCoord.y, currArea.originInTile.x, currArea.originInTile.y);
	}
}

bool ExploreMovementGenerator::getReachableTileInExplorArea(ExplorArea const& area, TileCoord& result)
{
	MapData* mapData = m_owner->getMap()->getMapData();
	Point pos = m_owner->computeExplorAreaPosition(area);;
	TileCoord coord(mapData->getMapSize(), pos);
	std::vector<TileCoord> foundCoordList;
	m_owner->getMap()->findNearestOpenTileList(coord, foundCoordList);
	NS_ASSERT(!foundCoordList.empty());
	Point foundPos = foundCoordList.front().computePosition(mapData->getMapSize());
	if (m_owner->isExplorAreaContainsPoint(area, foundPos))
	{
		result = foundCoordList.front();
		return true;
	}

	return false;
}

bool ExploreMovementGenerator::getSafeTileInExplorArea(ExplorArea const& area, TileCoord& result)
{
	MapData* mapData = m_owner->getMap()->getMapData();
	TileCoord reachableCoord;
	if (this->getReachableTileInExplorArea(area, reachableCoord))
	{
		if (!m_owner->getMap()->isSafeDistanceMaintained(reachableCoord))
		{
			TileCoord const& safeZoneCenter = m_owner->getMap()->getSafeZoneCenter();
			TileCoord newCoord;
			int32 side = m_owner->getMap()->getCurrentSafeZoneRadius() - m_owner->getMap()->getCurrentSafeDistance();
			int32 minX = safeZoneCenter.x - side;
			int32 minY = safeZoneCenter.y - side;
			int32 maxX = safeZoneCenter.x + side;
			int32 maxY = safeZoneCenter.y + side;
			newCoord.x = std::min(maxX, std::max(reachableCoord.x, minX + 1));
			newCoord.y = std::min(maxY, std::max(reachableCoord.y, minY + 1));

			Point pos = newCoord.computePosition(mapData->getMapSize());
			if (m_owner->isExplorAreaContainsPoint(area, pos))
			{
				//NS_LOG_DEBUG("movement.generator.explore", "NotWithinSafeDistance: %d,%d reachableCoord: %d,%d newCoord: %d,%d", area.x, area.y, reachableCoord.x, reachableCoord.y, newCoord.x, newCoord.y);

				std::vector<TileCoord> foundCoordList;
				m_owner->getMap()->findNearestOpenTileList(newCoord, foundCoordList);
				NS_ASSERT(!foundCoordList.empty());
				for (auto const& foundCoord : foundCoordList)
				{
					//NS_LOG_DEBUG("movement.generator.explore", "foundCoord: %d,%d", foundCoord.x, foundCoord.y);

					Point foundPos = foundCoord.computePosition(mapData->getMapSize());
					if (m_owner->isExplorAreaContainsPoint(area, foundPos))
					{
						if (!m_owner->getMap()->isDangerousDistrict(foundCoord)
							&& m_owner->getMap()->isSafeDistanceMaintained(foundCoord)
							/*&& mapData->isSameDistrict(foundCoord, reachableCoord)*/)
						{
							result = foundCoord;
							return true;
						}
					}
				}
			}
		}
		else
		{
			if (!m_owner->getMap()->isDangerousDistrict(reachableCoord))
			{
				result = reachableCoord;
				return true;
			}
		}
	}

	return false;
}

bool ExploreMovementGenerator::moveAndExploreNewArea(ExplorArea const& startArea, TileCoord const& currCoord, ExplorArea const& goalArea, ExplorArea& newArea)
{
	MapData* mapData = m_owner->getMap()->getMapData();
	TileCoord targetCoord = goalArea.originInTile;
	TileCoord nextStep;
	newArea = ExplorArea::INVALID;
	TargetStepGenerator::StepInfo stepInfo;
	if (m_stepGenerator.step(nextStep, currCoord, targetCoord, &stepInfo))
	{
		if (m_owner->getMap()->isSafeDistanceMaintained(nextStep))
		{
			Point pos = nextStep.computePosition(mapData->getMapSize());
			ExplorArea currArea = m_owner->getExplorArea(pos);
			if (this->getSafeTileInExplorArea(currArea, currArea.originInTile))
			{
				NS_ASSERT(m_owner->getMap()->isSafeDistanceMaintained(currArea.originInTile));
				NS_ASSERT(!m_owner->getMap()->isDangerousDistrict(currArea.originInTile));
				if (m_owner->getExplorOrder(currArea) == 0
					&& !m_owner->isExplorAreaExcluded(currArea)
					&& mapData->isSameDistrict(currArea.originInTile, startArea.originInTile))
				{
					NS_ASSERT(currArea != startArea);
					newArea = currArea;
				}
			}
		}

		if (!m_owner->getMap()->isWithinSafeZone(nextStep))
			NS_LOG_DEBUG("movement.generator.explore", "Step (%d,%d) is in danger.", nextStep.x, nextStep.y);

		this->moveStart();
		m_owner->getMoveSpline()->moveByStep(nextStep);
		return true;
	}

	return false;
}

bool ExploreMovementGenerator::moveAndExploreSafeArea(ExplorArea const& startArea, TileCoord const& currCoord, TileCoord& nextStep, ExplorArea& safeArea)
{
	MapData* mapData = m_owner->getMap()->getMapData();
	TileCoord const& safeZoneCenter = m_owner->getMap()->getSafeZoneCenter();
	safeArea = ExplorArea::INVALID;
	TargetStepGenerator::StepInfo stepInfo;
	if (m_stepGenerator.step(nextStep, currCoord, safeZoneCenter, &stepInfo))
	{
		if (m_owner->getMap()->isSafeDistanceMaintained(nextStep) && (!mapData->isSameDistrict(startArea.originInTile, nextStep)
			|| mapData->isSameDistrict(m_owner->getMap()->getSafeZoneCenter(), nextStep)))
		{
			Point pos = nextStep.computePosition(mapData->getMapSize());
			ExplorArea currArea = m_owner->getExplorArea(pos);
			if (this->getSafeTileInExplorArea(currArea, currArea.originInTile))
			{
				NS_ASSERT(m_owner->getMap()->isSafeDistanceMaintained(currArea.originInTile));
				NS_ASSERT(!m_owner->getMap()->isDangerousDistrict(currArea.originInTile));
				if (!m_owner->isExplorAreaExcluded(currArea)
					&& mapData->isSameDistrict(currArea.originInTile, nextStep))
				{
					safeArea = currArea;
				}
			}
		}

		this->moveStart();
		m_owner->getMoveSpline()->moveByStep(nextStep);
		return true;
	}

	return false;
}

bool ExploreMovementGenerator::move(TileCoord const& currCoord, TileCoord const& goalCoord)
{
	TileCoord nextStep;
	TargetStepGenerator::StepInfo stepInfo;
	if (m_stepGenerator.step(nextStep, currCoord, goalCoord, &stepInfo))
	{
		if (!m_owner->getMap()->isWithinSafeZone(nextStep))
			NS_LOG_DEBUG("movement.generator.explore", "Step (%d,%d) is in danger.", nextStep.x, nextStep.y);

		this->moveStart();
		m_owner->getMoveSpline()->moveByStep(nextStep);
		return true;
	}

	return false;
}

void ExploreMovementGenerator::finish()
{
	if (m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		m_owner->getMoveSpline()->stop();
		m_owner->clearUnitState(UNIT_STATE_MOVING);
	}
}

void ExploreMovementGenerator::initialize()
{
	TileCoord currCoord(m_owner->getData()->getMapData()->getMapSize(), m_owner->getData()->getPosition());
	if (m_owner->isInPatrol())
	{
		m_goalPatrolPoint = currCoord;
		m_explorState = EXPLOR_STATE_PATROLLING;
		NS_LOG_DEBUG("movement.generator.explore", "Initialize ExploreMovementGenerator goalPatrolPoint: %d,%d", m_goalPatrolPoint.x, m_goalPatrolPoint.y);
	}
	else
	{
		ExplorArea currArea = m_owner->getExplorArea(m_owner->getData()->getPosition());
		currArea.originInTile = currCoord;
		m_currExplorArea = currArea;
		m_goalExplorArea = currArea;
		m_owner->setGoalExplorArea(ExplorArea::INVALID);
		m_owner->setCurrentWaypointExplorArea(ExplorArea::INVALID);
		NS_LOG_DEBUG("movement.generator.explore", "Initialize ExploreMovementGenerator currArea: %d,%d (originInTile: %d,%d)", currArea.x, currArea.y, currArea.originInTile.x, currArea.originInTile.y);
	}
}

void ExploreMovementGenerator::sendMoveStart()
{
	MovementInfo movement = m_owner->getData()->getMovementInfo();
	m_owner->sendMoveOpcodeToNearbyPlayers(MSG_MOVE_START, movement);
}

void ExploreMovementGenerator::sendMoveStop()
{
	MovementInfo movement = m_owner->getData()->getMovementInfo();
	m_owner->sendMoveOpcodeToNearbyPlayers(MSG_MOVE_STOP, movement);
}
