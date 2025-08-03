#ifndef __ROBOT_MOVE_SPLINE_H__
#define __ROBOT_MOVE_SPLINE_H__

#include <list>

#include "utilities/Timer.h"
#include "game/tiles/TileCoord.h"
#include "MoveSpline.h"

class Robot;
class MoveSegment;

class RobotMoveSpline: public MoveSpline
{
public:
	RobotMoveSpline(Robot* owner);
	~RobotMoveSpline();

	void update(NSTime diff) override;

	void moveByStep(TileCoord const& step);
	void moveByPosition(Point const& position);
	void stop() override;

	MoveSegment* getCurrSegment() const { return m_currSegment; }

private:
	void popStepAndWalk();
	void sendMoveSync(Point const& pos);
	void sendRelocateLocator(Point const& pos);

	Robot* m_owner;

	MoveSegment* m_currSegment;
	std::list<Point> m_path;
	int32 m_realMoveSpeed;
};

#endif // __ROBOT_MOVE_SPLINE_H__