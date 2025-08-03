#include "RewardManager.h"

#include "game/behaviors/Unit.h"

class AwardeeOrderPred
{
public:
	AwardeeOrderPred() { }
	bool operator() (AwardeeReference const* a, AwardeeReference const* b) const
	{
		return a->getDamagePoints() > b->getDamagePoints();
	}
};


RewardManager::RewardManager(Unit* owner):
	m_owner(owner),
	m_isDirty(false),
	m_aggDamage(0)
{
}


RewardManager::~RewardManager()
{
	m_owner = nullptr;
	this->removeAllAwardees();
}

void RewardManager::addDamage(Unit* attacker, int32 damage)
{
	if (damage > 0)
	{
		AwardeeReference* awardeeRef = this->getReferenceByTarget(attacker);
		if (!awardeeRef)
		{
			awardeeRef = new AwardeeReference(attacker, this);
			m_awardeeList.emplace_back(awardeeRef);
		}

		awardeeRef->addDamage(damage);
		m_aggDamage += damage;
	}
}

void RewardManager::awardAllAwardees(Unit** champion)
{
	this->update();

	if(m_awardeeList.size() > 1)
		m_awardeeList.sort(AwardeeOrderPred());

	if(champion)
		*champion = nullptr;

	int32 aggDmg = m_aggDamage;
	auto it = m_awardeeList.begin();
	if (it != m_awardeeList.end())
	{
		AwardeeReference* championRef = *it;
		if (champion)
			*champion = championRef->getTarget();

		for (; it != m_awardeeList.end(); ++it)
		{
			AwardeeReference* ref = *it;
			NS_ASSERT(ref->isValid());
			if (ref->getDamagePoints() > 0)
			{
				ref->award();
				aggDmg -= ref->getDamagePoints();
			}
		}
	}

	NS_ASSERT(aggDmg >= 0, "Aggregate damage should not be negative.");

	m_aggDamage = 0;
	this->removeAllAwardees();
}

void RewardManager::removeAllAwardees()
{
	for (auto it = m_awardeeList.begin(); it != m_awardeeList.end();)
	{
		AwardeeReference* ref = *it;
		ref->unlink();
		it = m_awardeeList.erase(it);
		delete ref;
	}
	m_aggDamage = 0;
}

void RewardManager::update()
{ 
	for (auto it = m_awardeeList.begin(); it != m_awardeeList.end(); ++it)
	{
		AwardeeReference* ref = *it;
		ref->update();
	}

	if (m_isDirty)
	{
		for (auto it = m_awardeeList.begin(); it != m_awardeeList.end(); )
		{
			AwardeeReference* ref = *it;
			if (!ref->isValid() || ref->canDelete())
			{
				if(ref->getDamagePoints() > 0)
					m_aggDamage -= ref->getDamagePoints();

				it = m_awardeeList.erase(it);
				delete ref;
			}
			else
				++it;
		}
	}

	m_isDirty = false;
}

AwardeeReference* RewardManager::getReferenceByTarget(Unit* target) const
{
	for (auto it = m_awardeeList.begin(); it != m_awardeeList.end(); ++it)
	{
		if ((*it)->getTarget() == target)
			return (*it);
	}
	return nullptr;
}
