#include "RobotAI.h"

#include "game/behaviors/Robot.h"

RobotAI::RobotAI(Robot* robot) :
	m_me(robot),
	m_action(AI_ACTION_TYPE_NONE)
{
}

RobotAI::~RobotAI()
{
	m_me = nullptr;
}

void RobotAI::resetAI()
{
	m_action = AI_ACTION_TYPE_NONE;
	m_me->getData()->setAIActionType(AI_ACTION_TYPE_NONE);
}

void RobotAI::setAIAction(AIActionType action)
{
	if (m_action != action)
	{
		m_action = action;
		m_me->getData()->setAIActionType(action);
	}
}

void RobotAI::clearAIAction(AIActionType action)
{
	if (m_action == action)
	{
		m_action = AI_ACTION_TYPE_NONE;
		m_me->getData()->setAIActionType(AI_ACTION_TYPE_NONE);
	}
}
