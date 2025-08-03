#ifndef __TRAINING_ROBOT_AI_H__
#define __TRAINING_ROBOT_AI_H__

#include "utilities/Timer.h"
#include "game/movement/generators/MovementGenerator.h"
#include "RobotAI.h"
#include "TargetSelector.h"

class TrainingRobotAI : public RobotAI
{
public:
	TrainingRobotAI(Robot* robot);
	~TrainingRobotAI();

	RobotAIType getType() const override { return ROBOTAI_TYPE_TRAINING; }

	void updateAI(NSTime diff) override;
	void resetAI() override;

	void combatDelayed(Unit* victim) override;
	void combatStart(Unit* victim) override;

	void moveInLineOfSight(Unit* who) override;
	void moveInLineOfSight(Item* item) override;
private:
	void updateReactionTimer(NSTime diff);

	void updateCombat();

	void doCombat();

	void applyCombatMotion(Unit* target);

	DelayTimer m_reactionTimer;
	bool m_isReactionTimeDirty;

	TargetSelector m_targetSelector;
};


#endif // __TRAINING_ROBOT_AI_H__

