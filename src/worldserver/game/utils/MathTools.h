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
	// Calculate the radian between two points.
	static float computeAngleInRadians(Point const& p1, Point const& p2);

	static bool isDegreesInRange(float deg, float center, float swingDeg);
	static float degrees2Radians(float deg);
	static float radians2Degrees(float rad);

	// // Find a point along a line from p1 to p2 that is a certain distance from p1.
	static Point findPointAlongLine(Point const& p1, Point const& p2, float distance);
	static bool isPointInsideCircle(Point const& center, float radius, Point const& p);
	// Find the intersection point between the circle and the line from p1 to p2.
	static bool findIntersectionsLineCircle(Point const& p1, Point const& p2, Point const& center, float radius, std::vector<Point>& result);

	static bool findTangentPointsInCircle(Point const& p, Point const& center, float radius, Point& tp1, Point& tp2);
	// Calculate the minimum distance between a line segment from p1 to p2 and point p.
	static float minDistanceFromPointToSegment(Point const& p1, Point const& p2, Point const& p);

	// Find the point where line 1 and line 2 intersect.
	static bool findIntersectionTwoLines(Point const& l1p1, Point const& l1p2, Point const& l2p1, Point const& l2p2, Point& result);
	// Find the intersection point of two tangents on a circle.
	static bool findIntersectionTwoTangentsOnCircle(Point const& center, Point const& tp1, Point const& tp2, Point& result);
	// If d < 0, then the point is on one side of the line. 
	// If d > 0, then the point is on the other side. 
	// If d = 0, then the point is exactly on the line.
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