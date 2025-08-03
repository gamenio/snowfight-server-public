#include "TargetStepGenerator.h"

#include "game/behaviors/Unit.h"
#include "logging/Log.h"

TargetStepGenerator::TargetStepGenerator(Unit* owner):
	m_owner(owner),
    m_jpsPlus(nullptr)
{
	m_jpsPlus = m_owner->getMap()->getJPSPlus();
	NS_ASSERT(m_jpsPlus);

	m_expectedStep = TileCoord::INVALID;
	m_target = TileCoord::INVALID;
}


TargetStepGenerator::~TargetStepGenerator()
{
	m_jpsPlus = nullptr;
}

bool TargetStepGenerator::step(TileCoord& nextStep, TileCoord const& currStep, TileCoord const& target, StepInfo* info)
{
	bool ret = false;

	if (info)
		(*info) = {};

	if (currStep != target)
	{
		if (m_target != target || currStep != m_expectedStep)
		{
			m_path.clear();
			m_expectedStep = TileCoord::INVALID;
			m_target = TileCoord::INVALID;

			if (m_jpsPlus->getPath(currStep, target, m_path))
			{
				m_target = target;
				if (info)
					(*info).isPathfinding = true;
			}
		}

		auto it = m_path.begin();
		if (it != m_path.end())
		{
			TileCoord const& currNode = *it;
			int32 xDiff = currNode.x - currStep.x;
			int32 yDiff = currNode.y - currStep.y;

			int32 xInc = 0;
			int32 yInc = 0;

			if (xDiff > 0)
				xInc = 1;
			else if (xDiff < 0)
				xInc = -1;

			if (yDiff > 0)
				yInc = 1;
			else if (yDiff < 0)
				yInc = -1;

			TileCoord newCoord;
			newCoord.x = currStep.x + xInc;
			newCoord.y = currStep.y + yInc;

			if (newCoord == currNode)
			{
				m_path.erase(it);
				if (info)
					(*info).isKeyNode = true;
			}

			m_expectedStep = newCoord;
			nextStep = newCoord;
			ret = true;
		}
	}

	return ret;
}