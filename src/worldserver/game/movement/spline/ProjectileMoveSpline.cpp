#include "ProjectileMoveSpline.h"

#include "game/behaviors/Projectile.h"
#include "game/behaviors/Player.h"
#include "utilities/TimeUtil.h"

static const float NEWTON_RAPHSON_ERROR = 0.0001f;
static const int32 NEWTON_RAPHSON_MAX_ITER = 10;

// https://en.wikipedia.org/wiki/B%C3%A9zier_curve
static inline float bezierat(float a, float b, float c, float t)
{
	return powf(1 - t, 2) * a + 2 * t*(1 - t)*b + powf(t, 2) *c;
}

float calcQuadBezierLength(float A, float B, float C, float t)
{
	float Scba = sqrt(C + t * (B + A * t));
	float AB_Sc = (2 * A * t * Scba + B * (Scba - sqrt(C)));
	float bbac = 0.f;
	float bac = B + 2 * sqrt(A) * sqrt(C);
	float baa = B + 2 * A * t + 2 * sqrt(A) * Scba;
	if (bac > 0.f && baa > 0.f)
		bbac = (B * B - 4 * A * C) * (log(bac) - log(baa));

	float Sa = 2 * sqrt(A) * AB_Sc;
	float l = (Sa + bbac) / (8 * pow(A, 1.5f));

	NS_ASSERT(!std::isinf(l));
	NS_ASSERT(!std::isnan(l));

	return l;
}

// https://blog.51cto.com/u_15458423/4808292
float calcUniformSpeedQuadBezierTime(float A, float B, float C, float t, float l)
{
	float t1 = t, t2;
	int32 iter = 0;
	do
	{
		++iter;
		float length = calcQuadBezierLength(A, B, C, t1);
		float speed = sqrt(A * t1 * t1 + B * t1 + C);
		NS_ASSERT(speed > 0);
		t2 = t1 - (length - l) / speed;
		NS_ASSERT(!std::isinf(t2));
		NS_ASSERT(!std::isnan(t2));
		float a = t1 - t2;
		if (fabs(t1 - t2) < NEWTON_RAPHSON_ERROR || iter >= NEWTON_RAPHSON_MAX_ITER)
			break;
		t1 = t2;
	} while (true);

	return t2;
}

ProjectileMoveSpline::ProjectileMoveSpline(Projectile* owner):
	m_owner(owner),
	m_duration(0),
	m_elapsed(0)
{
}

ProjectileMoveSpline::~ProjectileMoveSpline()
{
	m_owner = nullptr;
}

void ProjectileMoveSpline::update(NSTime diff)
{
	if (m_isFinished)
		return;

	int32 maxDiff = (int32)((MAX_STEP_LENGTH - 1) / PROJECTILE_SPEED * 1000);
	diff = std::min(maxDiff, diff);

	float updateDt = std::min(1.0f, m_elapsed / float(m_duration));

	// 求出在贝塞尔曲线上，到时间updateDt时移动到的点
	BezierCurveConfig const& config = m_owner->getData()->getTrajectory();
	float xa = 0;
	float xb = config.controlPoints[0].x;
	float xc = config.endPosition.x;

	float ya = 0;
	float yb = config.controlPoints[0].y;
	float yc = config.endPosition.y;

	float ax = xa - 2 * xb + xc;
	float ay = ya - 2 * yb + yc;
	float bx = 2 * xb - 2 * xa;
	float by = 2 * yb - 2 * ya;
	float A = 4 * (ax * ax + ay * ay);
	float B = 4 * (ax * bx + ay * by);
	float C = bx * bx + by * by;
	float l = updateDt * config.length;
	float t = calcUniformSpeedQuadBezierTime(A, B, C, updateDt, l);

	float x = bezierat(xa, xb, xc, t);
	float y = bezierat(ya, yb, yc, t);

	m_prevPosition = m_owner->getData()->getPosition();
	Point newPosition;
	newPosition.x = config.startPosition.x + x;
	newPosition.y = config.startPosition.y + y;
	m_owner->updatePosition(newPosition);
	m_owner->getData()->setElapsed(std::min(m_duration, m_elapsed));

#if NS_DEBUG
	float offset = m_prevPosition.getDistance(newPosition);
	NS_ASSERT(offset <= MAX_STEP_LENGTH);
#endif // NS_DEBUG

	if (m_elapsed < m_duration)
		m_elapsed += diff;
	else
		this->stop();
}

void ProjectileMoveSpline::move()
{
	if (!m_isFinished)
		return;

	m_isFinished = false;
	m_elapsed = 0;
	m_prevPosition = m_owner->getData()->getPosition();
	m_duration = m_owner->getData()->getDuration();
}

void ProjectileMoveSpline::stop()
{
	if (m_isFinished)
		return;

	m_isFinished = true;
}