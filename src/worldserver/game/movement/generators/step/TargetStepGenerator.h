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

	// 生成从起始位置（currStep）到目标位置（target）的路径，如果返回true则给出下一步（nextStep）的位置，
	// 如果返回false则说明出现下列情况之一：
	// - 已到达目标位置
	// - 不存在路径
	bool step(TileCoord& nextStep, TileCoord const& currStep, TileCoord const& target, StepInfo* info = nullptr);

private:
	Unit* m_owner;
	JPSPlus* m_jpsPlus;

	std::vector<TileCoord> m_path;
	TileCoord m_target;
	TileCoord m_expectedStep;
};


#endif // __TARGET_STEP_GENERATOR_H__

