#ifndef __IDLE_MOVEMENT_GENERATOR_H__
#define __IDLE_MOVEMENT_GENERATOR_H__

#include "utilities/Timer.h"
#include "game/tiles/TileArea.h"
#include "MovementGenerator.h"


class Robot;

class IdleMovementGenerator : public MovementGenerator
{
public:
	IdleMovementGenerator(Robot* owner):
		m_owner(owner)
	{
	}

	~IdleMovementGenerator() 
	{
		m_owner = nullptr;
	}

	MovementGeneratorType getType() const override { return MOVEMENT_GENERATOR_TYPE_IDLE; }
	bool update(NSTime diff) override { return true; }
	void finish() override {}

private:
	Robot* m_owner;
};

#endif // __IDLE_MOVEMENT_GENERATOR_H__