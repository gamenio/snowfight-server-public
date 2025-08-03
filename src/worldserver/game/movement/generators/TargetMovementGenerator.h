#ifndef __TARGET_MOVEMENT_GENERATOR_H__
#define __TARGET_MOVEMENT_GENERATOR_H__

#include "game/entities/MovementInfo.h"
#include "game/server/protocol/Opcode.h"
#include "game/behaviors/Robot.h"
#include "MovementGenerator.h"
#include "FollowerReference.h"

template<typename TARGET>
class TargetMovementGenerator: public MovementGenerator
{
public:
	TargetMovementGenerator(Robot* owner, TARGET* target) :
		m_owner(owner)
	{
		m_target.link(target, this);
	}

	virtual ~TargetMovementGenerator()
	{
		m_owner = nullptr;
	}

protected:
	void sendMoveStart()
	{
		MovementInfo movement = m_owner->getData()->getMovementInfo();
		m_owner->sendMoveOpcodeToNearbyPlayers(MSG_MOVE_START, movement);
	}

	void sendMoveStop()
	{
		MovementInfo movement = m_owner->getData()->getMovementInfo();
		m_owner->sendMoveOpcodeToNearbyPlayers(MSG_MOVE_STOP, movement);
	}

	Robot* m_owner;
	FollowerReference<TARGET> m_target;
};

#endif // __TARGET_MOVEMENT_GENERATOR_H__
