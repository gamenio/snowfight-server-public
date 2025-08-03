#ifndef __TRAJECTORY_GENERATOR_H__
#define __TRAJECTORY_GENERATOR_H__

#include "game/entities/Point.h"

struct BezierCurveConfig
{
	enum CurveOrder
	{
		QUADRATIC,
		CUBIC,
	};

	BezierCurveConfig() :
		order(QUADRATIC),
		length(0)
	{
	}

	CurveOrder order;
	Point startPosition;
	Point endPosition;
	std::vector<Point> controlPoints;
	float length;
};

enum TrajectoryType
{
	TRAJECTORY_TYPE_PROJECTILE,
	TRAJECTORY_TYPE_ITEM,
};

class TrajectoryGenerator
{
public:
	TrajectoryGenerator(TrajectoryType type, Point const& origin, Point const& destination);
	~TrajectoryGenerator();

	TrajectoryType getType() const { return m_type; }
	void compute();
	BezierCurveConfig& getBezierCurveConfig() { return m_config; }
	BezierCurveConfig const& getBezierCurveConfig() const { return m_config; }

private:
	void computeProjectileTrajectory();
	void computeItemTrajectory();

	TrajectoryType m_type;
	Point m_origin;
	Point m_destination;

	BezierCurveConfig m_config;
};

#endif // __TRAJECTORY_GENERATOR_H__