#ifndef __UNIT_HOSTILE_REFERENCE_H__
#define __UNIT_HOSTILE_REFERENCE_H__

#include "game/dynamic/reference/Reference.h"
#include "game/entities/ObjectGuid.h"

enum UnitThreatType
{
	UNITTHREAT_DISTANCE,				// 我与敌人的距离
	UNITTHREAT_ENEMY_HEALTH,			// 敌人的生命值
	UNITTHREAT_RECEIVED_DAMAGE,			// 敌人对我累计造成的伤害
	UNITTHREAT_ENEMY_CHARGED_POWER,		// 敌人蓄力攻击的威力
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