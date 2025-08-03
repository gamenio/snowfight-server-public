#include "RandomStepGenerator.h"

#include "game/behaviors/Unit.h"

RandomStepGenerator::RandomStepGenerator(Unit* owner) :
	m_owner(owner)
{
}

RandomStepGenerator::~RandomStepGenerator()
{
	m_owner = nullptr;
}

bool RandomStepGenerator::isWalkable(TileCoord const& position)
{
	MapData const* mapData = m_owner->getMap()->getMapData();
	if (!mapData->isValidTileCoord(position))
		return false;

	if (mapData->isWall(position))
		return false;

	if (m_owner->getMap()->isTileClosed(position))
		return false;

	return true;
}

bool RandomStepGenerator::step(TileCoord& nextStep)
{
	// 判断起点位置周围的8个节点中可到达的节点
	std::vector<TileCoord> walkableSteps;
	TileCoord tmp;
	TileCoord currStep(m_owner->getMap()->getMapData()->getMapSize(), m_owner->getData()->getPosition());

	bool top, bottom, left, right;

	// Top
	tmp.setTileCoord(currStep.x, currStep.y - 1);
	top = isWalkable(tmp);
	if (top)
		walkableSteps.emplace_back(tmp);

	// Bottom
	tmp.setTileCoord(currStep.x, currStep.y + 1);
	bottom = isWalkable(tmp);
	if (bottom)
		walkableSteps.emplace_back(tmp);

	// Left
	tmp.setTileCoord(currStep.x - 1, currStep.y);
	left = isWalkable(tmp);
	if (left)
		walkableSteps.emplace_back(tmp);

	// Right
	tmp.setTileCoord(currStep.x + 1, currStep.y);
	right = isWalkable(tmp);
	if (right)
		walkableSteps.emplace_back(tmp);

	// LeftTop
	if (left && top)
	{
		tmp.setTileCoord(currStep.x - 1, currStep.y - 1);
		if (isWalkable(tmp))
			walkableSteps.emplace_back(tmp);
	}

	// LeftBottom
	if (left && bottom)
	{
		tmp.setTileCoord(currStep.x - 1, currStep.y + 1);
		if (isWalkable(tmp))
			walkableSteps.emplace_back(tmp);
	}

	// RightTop
	if (right && top)
	{
		tmp.setTileCoord(currStep.x + 1, currStep.y - 1);
		if (isWalkable(tmp))
			walkableSteps.emplace_back(tmp);
	}

	// RightBottom
	if (right && bottom)
	{
		tmp.setTileCoord(currStep.x + 1, currStep.y + 1);
		if (isWalkable(tmp))
			walkableSteps.emplace_back(tmp);
	}

	if (!walkableSteps.empty())
	{
		int32 stepIndex = random(0, (int32)(walkableSteps.size() - 1));
		nextStep = walkableSteps.at(stepIndex);
		return true;
	}

	return false;
}