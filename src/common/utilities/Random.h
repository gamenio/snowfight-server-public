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

// 如果随机掷骰子符合指定几率（范围 0-100）则返回true
bool rollChance(float chance);

// 从容器中随机选择一个元素，其中每个元素被选择的几率不同
// 注意： 
//   每个元素的权重必须与容器中元素的顺序相同，调用者负责检查所有权重之和是否大于0
//   容器不能为空
template <typename CONTAINER>
inline typename CONTAINER::const_iterator randomWeightedContainerElement(CONTAINER const& container, std::vector<double> const& weights)
{
	uint32 dist = RandomHelper::randomWeighted<uint32>(weights);
	typename CONTAINER::const_iterator it = container.begin();
	std::advance(it, dist);
	return it;
}

#endif // __RANDOM_H__
