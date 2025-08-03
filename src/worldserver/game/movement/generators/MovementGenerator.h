#ifndef __MOVEMENT_GENERATOR_H__
#define __MOVEMENT_GENERATOR_H__

#include "Common.h"

enum MovementGeneratorType
{
	MOVEMENT_GENERATOR_TYPE_IDLE,
	MOVEMENT_GENERATOR_TYPE_SMART_CHASE,
	MOVEMENT_GENERATOR_TYPE_SIMPLE_CHASE,
	MOVEMENT_GENERATOR_TYPE_ESCAPE,
	MOVEMENT_GENERATOR_TYPE_POINT,
	MOVEMENT_GENERATOR_TYPE_SEEK,
	MOVEMENT_GENERATOR_TYPE_EXPLORE,
};

class MovementGenerator
{
public:
	MovementGenerator() { }
	virtual ~MovementGenerator() { }

	virtual MovementGeneratorType getType() const = 0;
	virtual bool update(NSTime diff) = 0;
	virtual void finish() = 0;
	virtual void reset() {}

};


#endif // __MOVEMENT_GENERATOR_H__
