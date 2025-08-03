#include "UnitHostileRefManager.h"



void UnitHostileRefManager::updateThreat(UnitThreatType type, float variant)
{
	UnitHostileReference* ref = getFirst();
	while (ref)
	{
		ref->updateThreat(type, variant);
		ref = ref->next();
	}
}
