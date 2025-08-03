#include "MathTools.h"

#include <float.h>

#include "utilities/Random.h"
#include "utilities/TimeUtil.h"


float MathTools::computeAngleInRadians(Point const& p1, Point const& p2)
{
	float dx = p2.x - p1.x;
	float dy = p2.y - p1.y;
	float rad = atan2(dy, dx);
	return rad;
}

bool MathTools::isDegreesInRange(float deg, float center, float swingDeg)
{
	bool boundary = false;

	float left = center - swingDeg;
	if (left < 0)
	{
		left = 360 + left;
		boundary = true;
	}

	float right = center + swingDeg;
	if (right > 360)
	{
		right = right - 360;
		boundary = true;
	}

	if (boundary)
		return deg > left || deg <= right;
	else
		return deg > left && deg <= right;
}

Point MathTools::findPointAlongLine(Point const& p1, Point const& p2, float distance)
{
	float dx = p2.x - p1.x;
	float dy = p2.y - p1.y;
	float angle = std::atan2(dy, dx);
	float offX = std::cos(angle) * distance;
	float offY = std::sin(angle) * distance;

	return p1 + Point(offX, offY);
}

bool MathTools::isPointInsideCircle(Point const& center, float radius, Point const& p)
{
	float dx = p.x - center.x;
	float dy = p.y - center.y;
	if ((dx * dx + dy * dy) <= radius * radius)
		return true;
	else
		return false;
}

// https://cp-algorithms.com/geometry/circle-line-intersection.html
bool MathTools::findIntersectionsLineCircle(Point const& p1, Point const& p2, Point const& center, float radius, std::vector<Point>& result)
{
	float cx = center.x;
	float cy = center.y;
	float x1 = p1.x;
	float y1 = p1.y;
	float x2 = p2.x;
	float y2 = p2.y;

	float r = radius;
	float a = y2 - y1;
	float b = x1 - x2;
	if (std::abs(a) < FLT_EPSILON && std::abs(b) < FLT_EPSILON)
		return false;

	float c = x2 * y1 - x1 * y2;

	float x0 = (b * b * cx - a * b * cy - a * c) / (a * a + b * b);
	float y0 = (a * a * cy - a * b * cx - b * c) / (a * a + b * b);
	float d0 = std::abs(a * cx + b * cy + c) / std::sqrt(a * a + b * b);
	if (d0 > r)
		return false;
	else if (std::abs(d0 - r) < FLT_EPSILON)
		result.emplace_back(x0, y0);
	else
	{
		float d = r * r - std::pow(a * cx + b * cy + c, 2) / (a * a + b * b);
		float mult = std::sqrt(d / (a * a + b * b));
		float ax, ay, bx, by;
		ax = x0 + b * mult;
		bx = x0 - b * mult;
		ay = y0 - a * mult;
		by = y0 + a * mult;
		result.emplace_back(ax, ay);
		result.emplace_back(bx, by);
	}

	return true;
}

// https://stackoverflow.com/questions/49968720/find-tangent-points-in-a-circle-from-a-point/49987361#49987361
bool MathTools::findTangentPointsInCircle(Point const& p, Point const& center, float radius, Point& tp1, Point& tp2)
{
	// from math import sqrt, acos, atan2, sin, cos
	float Px = p.x;
	float Py = p.y;
	float Cx = center.x;
	float Cy = center.y;
	float a = radius;

	float b = std::sqrt((Px - Cx)*(Px - Cx) + (Py - Cy)*(Py - Cy));  // hypot() also works here
	if (a > b)
		return false;

	float th = std::acos(a / b);  // angle theta
	float d = std::atan2(Py - Cy, Px - Cx);  // direction angle of point P from C
	float d1 = d + th;  // direction angle of point T1 from C
	float d2 = d - th;  // direction angle of point T2 from C

	tp1.x = Cx + a * std::cos(d1);
	tp1.y = Cy + a * std::sin(d1);
	tp2.x = Cx + a * std::cos(d2);
	tp2.y = Cy + a * std::sin(d2);

	return true;
}

// https://www.geeksforgeeks.org/minimum-distance-from-a-point-to-the-line-segment-using-vectors/
float MathTools::minDistanceFromPointToSegment(Point const& p1, Point const& p2, Point const& p)
{
	Point const& A = p1;
	Point const& B = p2;
	Point const& E = p;

	// vector AB
	float ABx = B.x - A.x;
	float ABy = B.y - A.y;

	// vector BP
	float BEx = E.x - B.x;
	float BEy = E.y - B.y;

	// vector AP
	float AEx = E.x - A.x;
	float AEy = E.y - A.y;

	// Variables to store dot product
	float AB_BE, AB_AE;

	// Calculating the dot product
	AB_BE = (ABx * BEx + ABy * BEy);
	AB_AE = (ABx * AEx + ABy * AEy);

	// Minimum distance from
	// point E to the line segment
	float reqAns = 0;

	// Case 1
	if (AB_BE > 0) {

		// Finding the magnitude
		float y = E.y - B.y;
		float x = E.x - B.x;
		reqAns = std::sqrt(x * x + y * y);
	}

	// Case 2
	else if (AB_AE < 0) {
		float y = E.y - A.y;
		float x = E.x - A.x;
		reqAns = std::sqrt(x * x + y * y);
	}

	// Case 3
	else {

		// Finding the perpendicular distance
		float x1 = ABx;
		float y1 = ABy;
		float x2 = AEx;
		float y2 = AEy;
		float mod = std::sqrt(x1 * x1 + y1 * y1);
		if (mod > 0)
			reqAns = std::abs(x1 * y2 - y1 * x2) / mod;
	}
	return reqAns;
}

// https://www.geeksforgeeks.org/program-for-point-of-intersection-of-two-lines/
bool MathTools::findIntersectionTwoLines(Point const& l1p1, Point const& l1p2, Point const& l2p1, Point const& l2p2, Point& result)
{
	// Line 1 represented as a1x + b1y = c1
	float a1 = l1p2.y - l1p1.y;
	float b1 = l1p1.x - l1p2.x;
	float c1 = a1*(l1p1.x) + b1*(l1p1.y);

	// Line 2 represented as a2x + b2y = c2
	float a2 = l2p2.y - l2p1.y;
	float b2 = l2p1.x - l2p2.x;
	float c2 = a2*(l2p1.x) + b2*(l2p1.y);

	float determinant = a1*b2 - a2*b1;

	if (determinant == 0)
	{
		// The lines are parallel. 
		return false;
	}
	else
	{
		result.x = (b2*c1 - b1*c2) / determinant;
		result.y = (a1*c2 - a2*c1) / determinant;
		return true;
	}
}

bool MathTools::findIntersectionTwoTangentsOnCircle(Point const& center, Point const& tp1, Point const& tp2, Point& result)
{
	// https://stackoverflow.com/questions/6484978/calculating-intersection-point-of-two-tangents-on-one-circle/6485035#6485035
	float Cx = center.x;
	float Cy = center.y;
	float Ax = tp1.x;
	float Ay = tp1.y;
	float Bx = tp2.x;
	float By = tp2.y;
	Point L1, L2;
	L1.x = Ax + (Ay - Cy);
	L1.y = Ay - (Ax - Cx);
	L2.x = Bx + (By - Cy);
	L2.y = By - (Bx - Cx);

	return findIntersectionTwoLines(tp1, L1, tp2, L2, result);
}

float MathTools::findPointOnWhichSideOfLine(Point const& p, Point const& p1, Point const& p2)
{
	// https://math.stackexchange.com/questions/274712/calculate-on-which-side-of-a-straight-line-is-a-given-point-located/274728#274728
	float d = (p.x - p1.x) * (p2.y - p1.y) - (p.y - p1.y) * (p2.x - p1.x);
	if (std::abs(d) < FLT_EPSILON)
		d = 0.f;
	return d;
}