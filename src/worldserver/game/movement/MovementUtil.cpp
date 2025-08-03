#include "MovementUtil.h"

#define BYPASS_TARGET_SEGMENT_DISTANCE			TILE_WIDTH			// 绕过目标的分段距离


bool MovementUtil::calcTileCoordToBypassTarget(Unit* agent, Unit* target, TileCoord const& goalCoord, TileCoord& result)
{
	MapData* mapData = agent->getMap()->getMapData();
	Point const& targetPos = target->getData()->getPosition();
	Point const& agentPos = agent->getData()->getPosition();
	Point goalPos = goalCoord.computePosition(mapData->getMapSize());

	Point A = targetPos;
	Point B = agentPos;
	Point C = goalPos;
	float radius = target->getData()->getAttackRange();

	float minR = std::min(A.getDistance(C), A.getDistance(B));
	if (minR < radius)
		radius = minR;

	float dist = MathTools::minDistanceFromPointToSegment(B, C, A);
	if (radius > dist)
	{
		bool found = false;

		Point BT1, BT2;
		found = MathTools::findTangentPointsInCircle(B, A, radius, BT1, BT2);
		NS_ASSERT(found);

		Point CT1, CT2;
		found = MathTools::findTangentPointsInCircle(C, A, radius, CT1, CT2);
		NS_ASSERT(found);

		float d1 = BT2.getDistance(CT1);
		float d2 = BT1.getDistance(CT2);
		Point intersection;
		if (d1 < d2)
			found = MathTools::findIntersectionTwoTangentsOnCircle(A, BT2, CT1, intersection);
		else
			found = MathTools::findIntersectionTwoTangentsOnCircle(A, BT1, CT2, intersection);

		if (found)
		{
			Point newPos = MathTools::findPointAlongLine(agentPos, intersection, BYPASS_TARGET_SEGMENT_DISTANCE);
			TileCoord newCoord(mapData->getMapSize(), newPos);
			if (mapData->isValidTileCoord(newCoord)
				&& !mapData->isWall(newCoord)
				&& !target->getMap()->isTileClosed(newCoord))
			{
				result = newCoord;
				return true;
			}
		}
	}

	return false;
}
