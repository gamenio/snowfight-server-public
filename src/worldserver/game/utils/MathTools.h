#ifndef __MATH_TOOLS_H__
#define __MATH_TOOLS_H__

#include "Common.h"
#include "game/entities/Point.h"


#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2			1.57079632679489661923   // pi/2
#endif


class MathTools
{
public:
	// 计算两点之间的弧度
	static float computeAngleInRadians(Point const& p1, Point const& p2);

	static bool isDegreesInRange(float deg, float center, float swingDeg);
	static float degrees2Radians(float deg);
	static float radians2Degrees(float rad);

	// 沿着p1到p2的一条直线找到距离p1一定距离的点
	static Point findPointAlongLine(Point const& p1, Point const& p2, float distance);
	static bool isPointInsideCircle(Point const& center, float radius, Point const& p);
	// 求圆与从p1到p2的一条直线的交点。
	static bool findIntersectionsLineCircle(Point const& p1, Point const& p2, Point const& center, float radius, std::vector<Point>& result);

	static bool findTangentPointsInCircle(Point const& p, Point const& center, float radius, Point& tp1, Point& tp2);
	// 计算从p1到p2的一条线段与点p之间的最小距离
	static float minDistanceFromPointToSegment(Point const& p1, Point const& p2, Point const& p);

	// 求直线1和直线2相交的点
	static bool findIntersectionTwoLines(Point const& l1p1, Point const& l1p2, Point const& l2p1, Point const& l2p2, Point& result);
	// 求圆上两条切线的交点
	static bool findIntersectionTwoTangentsOnCircle(Point const& center, Point const& tp1, Point const& tp2, Point& result);
	// 如果d<0则该点位于直线的一侧，如果d>0则该点位于另一侧。如果d=0则该点正好位于直线上。
	static float findPointOnWhichSideOfLine(Point const& p, Point const& p1, Point const& p2);

	static NSTime computeMovingTimeMs(float distance, int32 speed);
	static float computeMovingDist(NSTime time, int32 speed);
};


inline NSTime MathTools::computeMovingTimeMs(float distance, int32 speed)
{
	return static_cast<NSTime>(distance / speed * 1000);
}

inline float MathTools::computeMovingDist(NSTime time, int32 speed)
{
	return (float)time / 1000 * speed;
}

inline float MathTools::degrees2Radians(float deg)
{
	return deg * (float)M_PI / 180.0f;
}

inline float MathTools::radians2Degrees(float rad)
{
	float deg = rad * 180.0f / (float)M_PI;
	return deg;
}



#endif // __MATH_TOOLS_H__