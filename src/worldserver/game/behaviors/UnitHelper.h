#ifndef __UNIT_HELPER_H__
#define __UNIT_HELPER_H__

#include "Common.h"
#include "game/entities/Point.h"
#include "game/entities/DataUnit.h"

class BattleMap;

template<typename T>
class ValueBuilder
{
public:
	ValueBuilder(uint32 level, uint32 maxLevel, float gradient, T startValue, T endValue, int32 precision) :
		m_level(level),
		m_maxLevel(maxLevel),
		m_gradient(gradient),
		m_startValue(startValue),
		m_endValue(endValue),
		m_precision(precision)
	{
		NS_ASSERT(m_maxLevel > 1);
		NS_ASSERT(m_level > 0 && m_level <= m_maxLevel);
	}

	inline operator int32() const
	{
		return this->getInt();
	}

	inline operator float() const
	{
		return this->getFloat();
	}

	int32 getInt() const
	{
		int32 value = (int32)std::round(this->getValue());
		return value;
	}

	float getFloat() const
	{
		float p = std::pow(10.f, m_precision);
		float value = std::floor(this->getValue() * p + 0.5f) / p;
		return value;
	}

private:
	float getValue() const
	{
		float avg = (m_endValue - m_startValue) / (float)(m_maxLevel - 1);
		float delta = avg * m_gradient;
		float inc = avg - (m_maxLevel - 2) / 2.f * delta;

		float li = (float)(m_level - 1);
		float value = m_startValue + li * inc + (li - 1) * li / 2.f * delta;

		return value;
	}

	uint32 m_maxLevel;
	uint32 m_level;
	float m_gradient;
	T m_startValue;
	T m_endValue;
	int32 m_precision;
};

class UnitHelper
{
public:
	static Point computeLandingPosition(DataUnit const* launcher, float direction);
	static Point computeLandingPosition(Point const& launcherPos, float attackRange, float direction);

	static Point computeLaunchPosition(DataUnit const* launcher, Point const& targetPos);
	static Point computeLaunchPosition(MapData const* mapData, Point const& launcherPos, Point const& launchCenter, float launchRadiusInMap, Point const& targetPos);

	static bool checkLineOfSight(BattleMap* map, TileCoord const& fromCoord, TileCoord const& toCoord);
};



#endif // __UNIT_HELPER_H__