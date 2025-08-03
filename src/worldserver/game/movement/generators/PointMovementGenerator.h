#ifndef __POINT_MOVEMENT_GENERATOR_H__
#define __POINT_MOVEMENT_GENERATOR_H__

#include "game/behaviors/Robot.h"
#include "step/TargetStepGenerator.h"
#include "MovementGenerator.h"

class PointMovementGenerator : public MovementGenerator
{
public:
	PointMovementGenerator(Robot* owner, TileCoord const& point, bool isBypassEnemy);
	virtual ~PointMovementGenerator();

	MovementGeneratorType getType() const override { return MOVEMENT_GENERATOR_TYPE_POINT; }
	bool update(NSTime diff) override;
	void finish() override;
private:
	void initialize();

	void sendMoveStart();
	void sendMoveStop();

	void moveStart();
	void moveStop();

	Robot* m_owner;

	TileCoord m_point;
	TileCoord m_realPoint;
	bool m_isBypassEnemy;
	TileCoord m_closestToPointCoord;
	TargetStepGenerator m_targetStepGenerator;

};

#endif // __POINT_MOVEMENT_GENERATOR_H__