#include "UnitThreatManager.h"

#include "game/behaviors/Robot.h"


class ThreatOrderPred
{
public:
	ThreatOrderPred(bool ascending = false) : m_ascending(ascending) { }
	bool operator() (UnitHostileReference const* a, UnitHostileReference const* b) const
	{
		return m_ascending ? a->getThreat() < b->getThreat() : a->getThreat() > b->getThreat();
	}
private:
	const bool m_ascending;
};

UnitThreatManager::UnitThreatManager(Robot* owner):
	m_owner(owner),
	m_isDirty(false)
{
}


UnitThreatManager::~UnitThreatManager()
{
	m_owner = nullptr;
	this->removeAllThreats();
}


Unit* UnitThreatManager::getHostileTarget()
{
	this->update();

	if (UnitHostileReference* ref = this->selectNextHostileRef())
		return ref->getTarget();

	return nullptr;
}


void UnitThreatManager::addThreat(Unit* victim, UnitThreatType type, float variant)
{
	UnitHostileReference* hostileRef = this->getReferenceByTarget(victim);
	if (!hostileRef)
	{
		hostileRef = new UnitHostileReference(victim, this);
		auto it = m_threatList.emplace(m_threatList.end(), hostileRef);
		NS_ASSERT(it != m_threatList.end());
		auto ret = m_threatLookupTable.emplace(victim->getData()->getGuid(), it);
		NS_ASSERT(ret.second);
	}

	hostileRef->updateThreat(type, variant);
}

bool UnitThreatManager::isHostileTarget(Unit* victim) const
{
	auto it = m_threatLookupTable.find(victim->getData()->getGuid());
	return it != m_threatLookupTable.end();
}

void UnitThreatManager::updateAllThreats(UnitThreatType type, float variant)
{
	for (auto it = m_threatList.begin(); it != m_threatList.end(); ++it)
	{
		UnitHostileReference* hostileRef = *it;
		if(!hostileRef->isValid())
			continue;

		hostileRef->updateThreat(type, variant);
	}
}

void UnitThreatManager::update()
{ 
	if (m_isDirty)
	{
		for (auto it = m_threatList.begin(); it != m_threatList.end(); )
		{
			UnitHostileReference* hostileRef = *it;
			if (!hostileRef->isValid())
			{
				it = m_threatList.erase(it);
				auto nRemoved = m_threatLookupTable.erase(hostileRef->getTargetGuid());
				NS_ASSERT(nRemoved != 0);

				delete hostileRef;
			}
			else
				++it;
		}

		if (m_threatList.size() > 1)
			m_threatList.sort(ThreatOrderPred());
	}

	m_isDirty = false;
}

UnitHostileReference* UnitThreatManager::getReferenceByTarget(Unit* target) const
{
	auto it = m_threatLookupTable.find(target->getData()->getGuid());
	if (it != m_threatLookupTable.end())
	{
		auto const& listIt = (*it).second;
		return *listIt;
	}

	return nullptr;
}

void UnitThreatManager::removeAllThreats()
{
	for (auto it = m_threatList.begin(); it != m_threatList.end(); )
	{
		UnitHostileReference* hostileRef = *it;
		it = m_threatList.erase(it);
		auto nRemoved = m_threatLookupTable.erase(hostileRef->getTargetGuid());
		NS_ASSERT(nRemoved != 0);

		hostileRef->unlink();
		delete hostileRef;
	}
}

UnitHostileReference* UnitThreatManager::selectNextHostileRef()
{
	UnitHostileReference* currRef = nullptr;
	for (auto it = m_threatList.begin(); it != m_threatList.end(); ++it)
	{
		UnitHostileReference* ref = *it;
		NS_ASSERT(ref->isValid());
		Unit* victim = ref->getTarget();
		if (m_owner->canCombatWith(victim))
		{
			currRef = ref;
			break;
		}

	}
	return currRef;
}

