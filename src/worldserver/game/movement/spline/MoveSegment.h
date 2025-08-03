#ifndef __MOVE_SEGMENT_H__
#define __MOVE_SEGMENT_H__

#include "Common.h"
#include "game/entities/Point.h"

class Unit;

class MoveSegment
{
public:
	MoveSegment(Unit* target, NSTime duration, Point const& endPosition);
	~MoveSegment();

	Point const& getEndPosition() const { return m_endPosition; }
	Point const& getStartPosition() const { return m_startPosition; }

	void step(NSTime diff);
	bool isDone() const;

	// 设置移动速度比例，值必须大于零
	void setSpeedScale(float scale);

private:
	void update(float time);

	Unit* m_target;

	NSTime m_elapsed;
	NSTime m_realDuration;
	NSTime m_duration;
	float m_speedScale;

	Point m_positionDelta;
	Point m_endPosition;
	Point m_startPosition;
	Point m_prevPosition;
};


#endif //__MOVE_SEGMENT_H__
