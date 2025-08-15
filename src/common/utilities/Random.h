#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <random>

#include "Defines.h"


class RandomHelper
{
public:
    template<typename T>
    static T randomReal(T min, T max)
    {
        std::uniform_real_distribution<T> dist(min, max);
        auto& mt = RandomHelper::getEngine();
        return dist(mt);
    }
    
    template<typename T>
    static T randomInt(T min, T max)
    {
        std::uniform_int_distribution<T> dist(min, max);
        auto& mt = RandomHelper::getEngine();
        return dist(mt);
    }

	template <typename T>
	static T randomWeighted(std::vector<double> const& weights)
	{
		std::discrete_distribution<T> dist(weights.begin(), weights.end());
		auto& mt = RandomHelper::getEngine();
		return dist(mt);
	}

private:
    static std::mt19937& getEngine();
};

template<typename T>
inline T random(T min, T max) {
    return RandomHelper::randomInt<T>(min, max);
}

template<>
inline float random(float min, float max) {
    return RandomHelper::randomReal(min, max);
}

template<>
inline long double random(long double min, long double max) {
    return RandomHelper::randomReal(min, max);
}

template<>
inline double random(double min, double max) {
    return RandomHelper::randomReal(min, max);
}

// If the random dice roll matches the specified chance (range 0-100), return true
bool rollChance(float chance);

// Randomly select one element from the container, where each element has a different chance of being selected
// Note:
//   * The weight of each element must be the same as the order of the elements in the container, 
//     and the caller is responsible for checking whether the sum of all weights is greater than 0
//   * The container cannot be empty
template <typename CONTAINER>
inline typename CONTAINER::const_iterator randomWeightedContainerElement(CONTAINER const& container, std::vector<double> const& weights)
{
	uint32 dist = RandomHelper::randomWeighted<uint32>(weights);
	typename CONTAINER::const_iterator it = container.begin();
	std::advance(it, dist);
	return it;
}

#endif // __RANDOM_H__
