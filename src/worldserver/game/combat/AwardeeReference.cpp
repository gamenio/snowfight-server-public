#include "AwardeeReference.h"

#include "utilities/TimeUtil.h"
#include "game/behaviors/Unit.h"
#include "game/world/ObjectMgr.h"
#include "RewardManager.h"

#define AWARDEE_TIMEOUT_TIME			30000 //  Awardee timeout time, unit: milliseconds

AwardeeReference::AwardeeReference(Unit* unit, RewardManager* rewardManager) :
	m_damagePoints(0),
	m_startTime(0),
	m_isTimeoutEnabled(true),
	m_canDelete(false)
{
	this->link(unit, rewardManager);
	this->resetTimeout();
}

AwardeeReference::~AwardeeReference()
{
}

void AwardeeReference::addDamage(int32 damage)
{
	m_damagePoints += damage;

	this->resetTimeout();
}

void AwardeeReference::award()
{
	Unit* victim = this->getSource()->getOwner();
	Unit* attacker = this->getTarget();

	float reward = victim->calcReward(attacker, m_damagePoints);
	attacker->applyReward(victim, reward);
}

void AwardeeReference::update()
{
	if (!isValid())
		return;

	if (m_isTimeoutEnabled)
	{
		int64 diff = getUptimeMillis() - m_startTime;
		if (diff >= AWARDEE_TIMEOUT_TIME)
		{
			this->getSource()->setDirty(true);
			m_startTime = 0;
			m_isTimeoutEnabled = false;
			m_canDelete = true;
		}
	}
}

void AwardeeReference::resetTimeout()
{
	m_startTime = getUptimeMillis();
}

void AwardeeReference::buildLink()
{
	this->getTarget()->getAwardSourceManager()->insertFirst(this);
	this->getTarget()->getAwardSourceManager()->incSize();
}

void AwardeeReference::destroyLink()
{
	this->getTarget()->getAwardSourceManager()->decSize();
	this->getSource()->setDirty(true);
}