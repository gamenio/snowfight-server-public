#include "MoveSegment.h"

#include "game/behaviors/Unit.h"
#include "game/behaviors/Player.h"


MoveSegment::MoveSegment(Unit* target, NSTime duration, Point const& endPosition) :
	m_target(target),
	m_elapsed(0),
	m_realDuration(duration),
	m_duration(duration),
	m_speedScale(1.0f),
	m_endPosition(endPosition),
	m_startPosition(target->getData()->getPosition()),
	m_prevPosition(target->getData()->getPosition())
{
	m_positionDelta = m_endPosition - m_startPosition;
}


MoveSegment::~MoveSegment()
{
	m_target = nullptr;
}


void MoveSegment::update(float t)
{
	if (!m_target)
		return;

	m_prevPosition = m_target->getData()->getPosition();
	Point newPos = m_startPosition + (m_positionDelta * t);
    m_target->updatePosition(newPos);

#if NS_DEBUG
	float offset = m_prevPosition.getDistance(newPos);
	NS_ASSERT(offset <= MAX_STEP_LENGTH);
#endif // NS_DEBUG
}

void MoveSegment::step(NSTime diff)
{
	int32 maxDiff = (int32)((MAX_STEP_LENGTH - 1.0f) / MAX_MOVE_SPEED * 1000);
	diff = std::min(maxDiff, diff);

	m_elapsed += diff;

	float updateDt = std::max(0.0f,                                  // needed for rewind. elapsed could be negative
		std::min(1.0f, m_elapsed / (float)m_duration)
	);

	this->update(updateDt);
}


bool MoveSegment::isDone() const
{
	return m_elapsed >= m_duration;
}

void MoveSegment::setSpeedScale(float scale)
{
	if (m_speedScale == scale || scale <= 0.f)
		return;

	float duration = m_realDuration *  (1.f / scale);
	float ratio = std::max(0.0f, std::min(1.0f, m_elapsed / (float)m_duration));
	m_elapsed = (NSTime)(ratio * duration);
	m_duration = (NSTime)(duration);
	m_speedScale = scale;
}

