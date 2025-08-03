#include "Random.h"

std::mt19937& RandomHelper::getEngine()
{
    static std::random_device seedGen;
    static std::mt19937 engine(seedGen());
    return engine;
}


bool rollChance(float chance)
{
	bool hit = false;
	if (chance > 0.f)
	{
		float r = random(0.f, 100.0f);
		hit = r <= chance;
	}

	return hit;
}