#include "TrajectoryGenerator.h"

#include <float.h>

#include "game/utils/MathTools.h"
#include "logging/Log.h"

// Projectile bezier curve control point configuration
static const float PROJECTILE_CONTROL_INITIAL_ANGLE = 51.f;	// Initial angle of control point
static const float PROJECTILE_CONTROL_LENGTH = 45.f;		// Control point length

// Item bezier curve control point configuration
static const float ITEM_CONTROL_1_INITIAL_ANGLE = 90.f;		// Initial angle of control point 1
static const float ITEM_CONTROL_2_INITIAL_ANGLE = 70.f;		// Initial angle of control point 2

// https://blog.csdn.net/qq_33662689/article/details/108904133
// https://gist.github.com/tunght13488/6744e77c242cc7a94859#file-quadratic-bezier-js
float calcQuadBezierLength(Point const& p0, Point const& p1, Point const& p2)
{
	float ax = p0.x - 2 * p1.x + p2.x;
	float ay = p0.y - 2 * p1.y + p2.y;
	float bx = 2 * p1.x - 2 * p0.x;
	float by = 2 * p1.y - 2 * p0.y;
	float A = 4 * (ax * ax + ay * ay);
	float B = 4 * (ax * bx + ay * by);
	float C = bx * bx + by * by;

	float Sabc = 2 * sqrt(A + B + C);
	float A_2 = sqrt(A);
	float A_32 = 2 * A * A_2;
	float C_2 = 2 * sqrt(C);
	float BA = B / A_2;

	float Laba = 0.f;
	float BAC = BA + C_2;
	if (BAC != 0.f)
	{
		float ABA = (2 * A_2 + BA + Sabc) / BAC;
		if (ABA > 0.f)
			Laba = log(ABA);
	}

	float l = ((A_32 * Sabc + A_2 * B * (Sabc - C_2) + (4 * C * A - B * B) * Laba) / (4 * A_32));
	NS_ASSERT(!std::isinf(l));
	NS_ASSERT(!std::isnan(l));

	return l;
}

TrajectoryGenerator::TrajectoryGenerator(TrajectoryType type, Point const& origin, Point const& destination) :
	m_type(type),
	m_origin(origin),
	m_destination(destination)
{
}

TrajectoryGenerator::~TrajectoryGenerator() 
{
}

void TrajectoryGenerator::compute()
{
	switch (m_type)
	{
	case TRAJECTORY_TYPE_PROJECTILE:
		this->computeProjectileTrajectory();
		break;
	case TRAJECTORY_TYPE_ITEM:
		this->computeItemTrajectory();
		break;
	default:
		break;
	}
}

void TrajectoryGenerator::computeProjectileTrajectory()
{
	float dist = m_destination.getDistance(m_origin);
	float dir = std::atan2(m_destination.y - m_origin.y, m_destination.x - m_origin.x);
	float ctrlRad = MathTools::degrees2Radians(PROJECTILE_CONTROL_INITIAL_ANGLE);

	// Transform dir to the following form:
	//      0
	//  /   |   \
	//-90 --+-- +90
	//  \   |   /
	//      0
	float rDir = (float)M_PI_2 - std::fabs(dir);
	float ar = rDir / (float)M_PI_2; // Find the ratio of the angles, (0°~±90°)/R90=(0~±1.0)

	float lr = std::min(1.0f, dist / PROJECTILE_CONTROL_LENGTH * 0.5f); // Find the ratio of the length of the control point to the distance
	float angle = ctrlRad * ar * lr;
	float len = PROJECTILE_CONTROL_LENGTH * lr;
	float dx, dy;
	if (std::abs(angle) < FLT_EPSILON)
	{
		float dl = dist - len;
		dx = std::cos(dir) * dl;
		dy = std::sin(dir) * dl;
	}
	else
	{
		float G = std::cos(angle) * len;
		float F = std::sin(angle) * len;
		float a = std::atan2(F, dist - G);
		float Fa = F / std::sin(a);
		float A = dir + a;
		dx = std::cos(A) * Fa;
		dy = std::sin(A) * Fa;
	}

	m_config.order = BezierCurveConfig::QUADRATIC;
	m_config.startPosition = m_origin;
	m_config.endPosition = m_destination - m_origin;
	m_config.controlPoints.emplace_back(dx, dy);

	float length = calcQuadBezierLength(Point::ZERO, m_config.controlPoints[0], m_config.endPosition);
	m_config.length = length;
}

void TrajectoryGenerator::computeItemTrajectory()
{
	float dist = m_destination.getDistance(m_origin);
	float dir = std::atan2(m_destination.y - m_origin.y, m_destination.x - m_origin.x);
	float ctrl1Rad = MathTools::degrees2Radians(ITEM_CONTROL_1_INITIAL_ANGLE);
	float ctrl2Rad = MathTools::degrees2Radians(ITEM_CONTROL_2_INITIAL_ANGLE);

	Point ctrl1;
	ctrl1.x = std::cos(ctrl1Rad) * dist;
	ctrl1.y = std::sin(ctrl1Rad) * dist;

	// Transform dir to the following form:
	//     ±180
	//  /   |   \
	//-90 --+-- +90
	//  \   |   /
	//      0
	float rDir = dir + (float)M_PI_2;
	if (rDir > (float)M_PI)
		rDir = rDir - (float)M_PI - (float)M_PI;
	float ar = rDir / (float)M_PI_2; // Find the ratio of the angles, (0°~±180°)/R90=(0~±2.0)
	float angle; // Calculate the angle range: 0°(ar=0)~± Initial angle (ar=±1.0)~±180°(ar=±2.0)
	if (std::fabs(ar) > 1.0f)
	{
		angle = ctrl2Rad + ((float)M_PI - ctrl2Rad) * (std::fabs(ar) - 1.0f);
		angle = ar < 0.f ? -angle : angle;
	}
	else
		angle = ctrl2Rad * ar;

	float len = 2 * dist * std::sin(angle / 2.f); // Formula for finding the base of an isosceles triangle
	float A = dir + ((float)M_PI - angle) / 2.f;
	Point ctrl2;
	ctrl2.x = std::cos(A) * len;
	ctrl2.y = std::sin(A) * len;

	m_config.order = BezierCurveConfig::CUBIC;
	m_config.startPosition = m_origin;
	m_config.endPosition = m_destination - m_origin;
	m_config.controlPoints.emplace_back(ctrl1);
	m_config.controlPoints.emplace_back(ctrl2);
}
