#ifndef __SMART_CHASE_MOVEMENT_GENERATOR_H__
#define __SMART_CHASE_MOVEMENT_GENERATOR_H__

#include "step/TargetStepGenerator.h"
#include "step/RandomStepGenerator.h"
#include "TargetMovementGenerator.h"
#include "game/behaviors/ItemBox.h"

enum DodgeDirection
{
	DODGE_NONE,
	DODGE_CLOCKWISE,
	DODGE_ANTICLOCKWISE,
};

template<typename TARGET>
class SmartChaseMovementGenerator: public TargetMovementGenerator<TARGET>
{
	static int32 DODGE_DURATION_MIN;
	static int32 DODGE_DURATION_MAX;
public:
	SmartChaseMovementGenerator(Robot* owner, TARGET* target);
	virtual ~SmartChaseMovementGenerator();

	MovementGeneratorType getType() const override { return MOVEMENT_GENERATOR_TYPE_SMART_CHASE; }
	bool update(NSTime diff) override;
	void finish() override;

private:
	void initialize();

	void moveStart();
	void moveStop();

	TileCoord calcDodgeTileCoordAroundTarget(TARGET* target, DodgeDirection dir, bool isKeepDistance);
	TileCoord calcDodgeTileCoordToTarget(TARGET* target, DodgeDirection dir);
	TileCoord getClosestTileCoordToTarget(TARGET* target);
	DodgeDirection calcDodgeDirection(Projectile* proj);
	DodgeDirection calcDodgeDirection(TARGET* target, Point const& goalPos);

	void updateDodgeDuration();

	TargetStepGenerator m_targetStepGenerator;
	RandomStepGenerator m_randomStepGenerator;

	bool m_isChasing;
	DelayTimer m_dodgeTimer;
	IntervalTimer m_reactionTimer;
	DodgeDirection m_currDodgeDirection;
	DodgeDirection m_nextDodgeDirection;
	bool m_isLocked;
};


#endif // __SMART_CHASE_MOVEMENT_GENERATOR_H__

