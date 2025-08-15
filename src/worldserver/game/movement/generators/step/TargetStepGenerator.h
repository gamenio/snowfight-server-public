#ifndef __TARGET_STEP_GENERATOR_H__
#define __TARGET_STEP_GENERATOR_H__

#include "game/tiles/TileCoord.h"
#include "game/movement/pathfinding/JPSPlus.h"

class Unit;

class TargetStepGenerator
{
public:
	struct StepInfo
	{
		bool isPathfinding;
		bool isKeyNode;
		double time;
	};

	TargetStepGenerator(Unit* owner);
	~TargetStepGenerator();

	// Generate a path from the currStep to the target. If true is returned, the nextStep is given.
	// If false is returned, one of the following situations has occurred:
	// - The target position has been reached.
	// - No path exists.

	bool step(TileCoord& nextStep, TileCoord const& currStep, TileCoord const& target, StepInfo* info = nullptr);

private:
	Unit* m_owner;
	JPSPlus* m_jpsPlus;

	std::vector<TileCoord> m_path;
	TileCoord m_target;
	TileCoord m_expectedStep;
};


#endif // __TARGET_STEP_GENERATOR_H__

