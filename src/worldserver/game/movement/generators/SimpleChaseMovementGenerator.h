#ifndef __SIMPLE_CHASE_MOVEMENT_GENERATOR_H__
#define __SIMPLE_CHASE_MOVEMENT_GENERATOR_H__

#include "step/TargetStepGenerator.h"
#include "step/RandomStepGenerator.h"
#include "TargetMovementGenerator.h"
#include "game/behaviors/ItemBox.h"

template<typename TARGET>
class SimpleChaseMovementGenerator : public TargetMovementGenerator<TARGET>
{
public:
	SimpleChaseMovementGenerator(Robot* owner, TARGET* target);
	virtual ~SimpleChaseMovementGenerator();

	MovementGeneratorType getType() const override { return MOVEMENT_GENERATOR_TYPE_SIMPLE_CHASE; }
	bool update(NSTime diff) override;
	void finish() override;

private:
	void moveStart();
	void moveStop();

	TargetStepGenerator m_targetStepGenerator;
	RandomStepGenerator m_randomStepGenerator;
};


#endif // __SIMPLE_CHASE_MOVEMENT_GENERATOR_H__

