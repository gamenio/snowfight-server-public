#ifndef __AWARDEE_REFERENCE_H__
#define __AWARDEE_REFERENCE_H__

#include "game/dynamic/reference/Reference.h"


class Unit;
class RewardManager;

class AwardeeReference: public Reference<Unit, RewardManager>
{
public:
	AwardeeReference(Unit* unit, RewardManager* rewardManager);
	~AwardeeReference();

	void addDamage(int32 damage);

	int32 getDamagePoints() const { return m_damagePoints; }
	void award();
	void update();

	bool canDelete() const { return m_canDelete; }

protected:
	void buildLink() override;
	void destroyLink() override;

private:
	void resetTimeout();

	int32 m_damagePoints;
	int64 m_startTime;
	bool m_isTimeoutEnabled;
	bool m_canDelete;
};

#endif // __AWARDEE_REFERENCE_H__