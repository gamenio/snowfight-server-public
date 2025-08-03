#ifndef __MOVE_SPLINE_H__
#define __MOVE_SPLINE_H__

#include "Common.h"

class MoveSpline
{
public:
	MoveSpline();
	virtual ~MoveSpline() = 0;

	virtual void update(NSTime diff) { }

	virtual bool isFinished() const { return m_isFinished; }
	virtual void stop() { }

protected:
	bool m_isFinished;
};

#endif //__MOVE_SPLINE_H__