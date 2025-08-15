#include "TrainingRobotAI.h"

#include "game/behaviors/Robot.h"
#include "game/behaviors/Item.h"
#include "game/behaviors/Projectile.h"
#include "game/movement/generators/MovementGenerator.h"
#include "game/world/World.h"
#include "game/world/ObjectMgr.h"
#include "game/utils/MathTools.h"
#include "logging/Log.h"
#include "utilities/Random.h"


TrainingRobotAI::TrainingRobotAI(Robot* robot) :
	RobotAI(robot),
	m_isReactionTimeDirty(true),
	m_targetSelector(robot)
{
}


TrainingRobotAI::~TrainingRobotAI()
{
}

void TrainingRobotAI::updateAI(NSTime diff)
{
	this->updateReactionTimer(diff);

	this->updateCombat();
}

void TrainingRobotAI::resetAI()
{
	m_isReactionTimeDirty = true;
	m_reactionTimer.setDuration(0);

	m_targetSelector.removeAllTargets();

	RobotAI::resetAI();
}

void TrainingRobotAI::combatDelayed(Unit* victim)
{
	m_targetSelector.addTarget(victim, TARGET_ACTION_COMBAT);
}

void TrainingRobotAI::combatStart(Unit* victim)
{
	bool targeted = m_me->setInCombatWith(victim);
	if ((targeted && this->getAIAction() == AI_ACTION_TYPE_COMBAT) || this->getAIAction() < AI_ACTION_TYPE_COMBAT)
		this->applyCombatMotion(victim);
}

void TrainingRobotAI::updateReactionTimer(NSTime diff)
{
	if (!m_me->getMap()->isInBattle())
		return;

	if (m_isReactionTimeDirty)
	{
		RobotProficiency const* proficiency = sObjectMgr->getRobotProficiency(m_me->getData()->getProficiencyLevel());
		NS_ASSERT(proficiency);
		NSTime dur = random(proficiency->minTargetReactionTime, proficiency->maxTargetReactionTime);
		m_reactionTimer.setDuration(dur);

		m_isReactionTimeDirty = false;
	}

	m_reactionTimer.update(diff);
	if (m_reactionTimer.passed())
	{
		this->doCombat();

		m_reactionTimer.reset();
	}
}

void TrainingRobotAI::updateCombat()
{
	if (!m_me->isInCombat())
		return;

	Unit* victim = m_me->selectVictim();
	if (victim)
	{
		this->combatStart(victim);
		Unit* victim = m_me->getVictim();
		NS_ASSERT(victim);
		m_me->setAttackTarget(victim);
		if (m_me->lockingTarget(victim))
		{
			if (m_me->isAttackReady())
				m_me->attack();
		}
	}
	else
	{
		m_me->deleteUnitThreatList();
		m_me->combatStop();
		m_me->getMotionMaster()->moveIdle();

		// When both oneself and nearby targets are stationary, updating the state of visibility 
		// allows one to actively search for targets within one's line of sight
		m_me->updateObjectVisibility();
	}
}

void TrainingRobotAI::doCombat()
{
	WorldObject* target = m_targetSelector.getVaildTarget(TARGET_ACTION_COMBAT);
	if (!target)
		return;

	this->combatStart(target->asUnit());

	m_targetSelector.removeTargetsForAction(TARGET_ACTION_COMBAT);
	m_isReactionTimeDirty = true;
}

void TrainingRobotAI::applyCombatMotion(Unit* target)
{
	m_me->getMotionMaster()->moveChase(target, false);

	this->setAIAction(AI_ACTION_TYPE_COMBAT);
	m_me->getData()->setAIActionState(m_me->getCombatState());
}

void TrainingRobotAI::moveInLineOfSight(Unit* who)
{
	if (m_me->getData()->hasReactFlag(REACT_FLAG_PASSIVE_AGGRESSIVE)
		|| m_me->getVictim())
	{
		return;
	}

	if (m_me->canCombatWith(who))
		this->combatDelayed(who);
}

void TrainingRobotAI::moveInLineOfSight(Item* item)
{

}