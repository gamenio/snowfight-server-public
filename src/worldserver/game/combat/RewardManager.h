#ifndef __REWARD_MANAGER_H__
#define __REWARD_MANAGER_H__

#include "game/entities/ObjectGuid.h"
#include "AwardeeReference.h"

class Unit;

class RewardManager
{
public:
	RewardManager(Unit* owner);
	~RewardManager();

	Unit* getOwner() const { return m_owner; }

	void addDamage(Unit* attacker, int32 damage);
	int32 getAggDamage() const { return m_aggDamage; }

	void awardAllAwardees(Unit** champion);
	void removeAllAwardees();

	void setDirty(bool isDirty) { m_isDirty = isDirty; }
	void update();

private:
	AwardeeReference* getReferenceByTarget(Unit* target) const;

	Unit* m_owner;
	bool m_isDirty;
	int32 m_aggDamage;
	std::list<AwardeeReference*> m_awardeeList;
};


#endif // __REWARD_MANAGER_H__

