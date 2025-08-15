#ifndef __UNIT_HOSTILE_REFERENCE_H__
#define __UNIT_HOSTILE_REFERENCE_H__

#include "game/dynamic/reference/Reference.h"
#include "game/entities/ObjectGuid.h"

enum UnitThreatType
{
	UNITTHREAT_DISTANCE,				// My distance from the enemy
	UNITTHREAT_ENEMY_HEALTH,			// The enemy's health
	UNITTHREAT_RECEIVED_DAMAGE,			// The total damage the enemy has dealing to me
	UNITTHREAT_ENEMY_CHARGED_POWER,		// The power of the enemy's charged attack
	MAX_UNITTHREAT_TYPES
};

class Unit;
class UnitThreatManager;

class UnitHostileReference: public Reference<Unit, UnitThreatManager>
{
public:
	UnitHostileReference(Unit* unit, UnitThreatManager* threatManager);
	~UnitHostileReference();

	ObjectGuid getTargetGuid() const { return m_targetGuid; }

	void updateThreat(UnitThreatType type, float variant);
	float getThreat() const { return m_threat; }

	UnitHostileReference* next() { return ((UnitHostileReference*)Reference<Unit, UnitThreatManager>::next()); }

protected:
	void buildLink() override;
	void destroyLink() override;

private:
	float calcThreatSum();

	ObjectGuid m_targetGuid;

	float m_totalDamage;
	float m_threatParts[MAX_UNITTHREAT_TYPES];
	float m_threat;
};

#endif // __UNIT_HOSTILE_REFERENCE_H__