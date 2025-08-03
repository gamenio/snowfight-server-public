#ifndef __ESCAPE_MOVEMENT_GENERATOR_H__
#define __ESCAPE_MOVEMENT_GENERATOR_H__

#include "step/TargetStepGenerator.h"
#include "step/RandomStepGenerator.h"
#include "TargetMovementGenerator.h"


class Robot;

class EscapeMovementGenerator : public TargetMovementGenerator<Unit>
{
public:
	EscapeMovementGenerator(Robot* owner, Unit* target);
	~EscapeMovementGenerator();

	MovementGeneratorType getType() const override { return MOVEMENT_GENERATOR_TYPE_ESCAPE; }
	bool update(NSTime diff) override;
	void finish() override;

private:
	void moveStart();
	void moveStop();

	TileCoord calcTileCoordToGetRidOfTarget(Unit* target, bool isReverse = false);
	TileCoord calcTileCoordToGetRidOfTarget(Unit* target, TileCoord const& goalCoord);

	Robot* m_owner;
	JPSPlus* m_jpsPlus;

	TileCoord m_closestToEscapeCoord;
	std::vector<TileCoord> m_path;
	bool m_isGoingToSafety;
	TargetStepGenerator m_targetStepGenerator;
	RandomStepGenerator m_randomStepGenerator;;
};

#endif // __ESCAPE_MOVEMENT_GENERATOR_H__