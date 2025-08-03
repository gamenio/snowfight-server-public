#ifndef __UNIT_AI_H__
#define __UNIT_AI_H__

#include "Common.h"

class UnitAI
{
public:
	UnitAI() { }
	virtual ~UnitAI() { }

	virtual void resetAI() {}
	virtual void updateAI(NSTime diff) = 0;
};

#endif //__UNIT_AI_H__

