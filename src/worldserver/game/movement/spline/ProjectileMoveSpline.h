#ifndef __PROJECTILE_MOVE_SPLINE_H__
#define __PROJECTILE_MOVE_SPLINE_H__

#include "game/entities/Point.h"
#include "MoveSpline.h"

class Projectile;

class ProjectileMoveSpline: public MoveSpline
{
public:
	ProjectileMoveSpline(Projectile* owner);
	~ProjectileMoveSpline();

	void update(NSTime diff) override;

	void move();
	void stop() override;

	NSTime getDuration() const { return m_duration; }
	NSTime getElapsed() const { return m_elapsed; }
	Point const& getPrevPosition() const { return m_prevPosition; }

private:
	Projectile* m_owner;

	NSTime m_duration;
	NSTime m_elapsed;
	Point m_prevPosition;
};

#endif // __PROJECTILE_MOVE_SPLINE_H__