#include "SimpleChaseMovementGenerator.h"

#include "logging/Log.h"
#include "game/server/protocol/Opcode.h"
#include "game/movement/spline/MoveSpline.h"
#include "MovementGenerator.h"

template<typename TARGET>
SimpleChaseMovementGenerator<TARGET>::SimpleChaseMovementGenerator(Robot* owner, TARGET* target) :
	TargetMovementGenerator<TARGET>(owner, target),
	m_targetStepGenerator(owner),
	m_randomStepGenerator(owner)
{
}

template<typename TARGET>
SimpleChaseMovementGenerator<TARGET>::~SimpleChaseMovementGenerator()
{
}


template<typename TARGET>
void SimpleChaseMovementGenerator<TARGET>::finish()
{
	if (this->m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		this->m_owner->getMoveSpline()->stop();
		this->m_owner->clearUnitState(UNIT_STATE_MOVING);
	}
}

template<typename TARGET>
bool SimpleChaseMovementGenerator<TARGET>::update(NSTime diff)
{
	if (!this->m_target.isValid() || !this->m_target->isInWorld())
		return false;

	if (!this->m_owner->isAlive())
		return false;

	NS_ASSERT(this->m_owner->isInCombat() || this->m_owner->isInUnlock());

	if (!this->m_owner->getMoveSpline()->isFinished())
		return true;

	// Target locked
	if (this->m_owner->lockingTarget(this->m_target.getTarget()))
	{
		this->moveStop();
	}
	// Start chasing the target
	else
	{
		TileCoord targetCoord(this->m_owner->getMap()->getMapData()->getMapSize(), this->m_target->getData()->getPosition());
		TileCoord currCoord(this->m_owner->getData()->getMapData()->getMapSize(), this->m_owner->getData()->getPosition());
		TileCoord nextStep;
		bool moving;
		if (currCoord == targetCoord)
			moving = m_randomStepGenerator.step(nextStep);
		else
			moving = this->m_targetStepGenerator.step(nextStep, currCoord, targetCoord);
		if (moving)
		{
			this->moveStart();
			this->m_owner->getMoveSpline()->moveByStep(nextStep);
		}
	}

	return true;
}

template<typename TARGET>
void SimpleChaseMovementGenerator<TARGET>::moveStart()
{
	if (!this->m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStart();
		this->m_owner->addUnitState(UNIT_STATE_MOVING);
	}
}

template<typename TARGET>
void SimpleChaseMovementGenerator<TARGET>::moveStop()
{
	if (this->m_owner->hasUnitState(UNIT_STATE_MOVING))
	{
		this->sendMoveStop();
		this->m_owner->setFacingToObject(this->m_target.getTarget());
		this->m_owner->clearUnitState(UNIT_STATE_MOVING);
	}
}

template class SimpleChaseMovementGenerator<Unit>;
template class SimpleChaseMovementGenerator<ItemBox>;
