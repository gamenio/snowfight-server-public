#ifndef __RANDOM_STEP_GENERATOR_H__
#define __RANDOM_STEP_GENERATOR_H__

#include "Common.h"
#include "game/tiles/TileCoord.h"

class Unit;

class RandomStepGenerator
{
public:
	RandomStepGenerator(Unit* owner);
	~RandomStepGenerator();

	bool step(TileCoord& nextStep);
private:
	bool isWalkable(TileCoord const& position);

	Unit* m_owner;
};


#endif // __RANDOM_STEP_GENERATOR_H__



