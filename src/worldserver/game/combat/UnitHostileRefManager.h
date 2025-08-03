#ifndef __UNIT_HOSTILE_REFMANAGER_H__
#define __UNIT_HOSTILE_REFMANAGER_H__

#include "game/dynamic/reference/RefManager.h"
#include "UnitHostileReference.h"

class Unit;
class UnitThreatManager;

class UnitHostileRefManager: public RefManager<Unit, UnitThreatManager>
{
public:
	UnitHostileReference* getFirst() { return ((UnitHostileReference*)RefManager<Unit, UnitThreatManager>::getFirst()); }

	void updateThreat(UnitThreatType type, float variant = 0.0f);
};

#endif // __UNIT_HOSTILE_REFMANAGER_H__
