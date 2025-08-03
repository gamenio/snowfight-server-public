#include "TargetSelector.h"

#include "game/behaviors/Robot.h"

class TargetOrderPred
{
public:
	TargetOrderPred() { }
	bool operator() (ObserverReference const* a, ObserverReference const* b) const
	{
		return a->getOrder() > b->getOrder();
	}
};

TargetSelector::TargetSelector(Robot* owner):
	m_owner(owner)
{
}


TargetSelector::~TargetSelector()
{
	m_owner = nullptr;
	this->removeAllTargets();
}


WorldObject* TargetSelector::getVaildTarget(TargetAction action)
{
	this->update(action);

	ObserverReference* ref = this->selectNextObserverRef(action);
	if(ref)
		return ref->getTarget();

	return nullptr;
}

void TargetSelector::addTarget(WorldObject* target, TargetAction action)
{
	ObserverReference* observerRef = this->getObserverRefByTarget(target, action);
	if (observerRef)
		return;

	observerRef = new ObserverReference(target, action, this);
	auto& observerRefList = m_actionTargets[action];
	auto it = observerRefList.emplace(observerRefList.end(), observerRef);
	NS_ASSERT(it != observerRefList.end());
	auto ret = m_actionTargetLookupTables[action].emplace(target->getData()->getGuid(), it);
	NS_ASSERT(ret.second);
}

void TargetSelector::update(TargetAction action)
{
	auto& observerRefList = m_actionTargets[action];
	for (auto refIt = observerRefList.begin(); refIt != observerRefList.end();)
	{
		ObserverReference* ref = (*refIt);
		if (!ref->isValid() || !this->canBeTarget(ref))
		{
			auto nRemoved = m_actionTargetLookupTables[action].erase(ref->getTargetGuid());
			NS_ASSERT(nRemoved != 0);
			refIt = observerRefList.erase(refIt);

			ref->unlink();
			delete ref;
		}
		else
		{
			ref->update();
			++refIt;
		}

	}
}

bool TargetSelector::canBeTarget(ObserverReference* targetRef)
{
	WorldObject* target = targetRef->getTarget();
	switch (targetRef->getTargetAction())
	{
	case TARGET_ACTION_COMBAT:
		return m_owner->canCombatWith(target->asUnit());
	case TARGET_ACTION_COLLECT:
		return m_owner->canCollect(target->asItem());
	case TARGET_ACTION_UNLOCK:
		return m_owner->canUnlock(target->asItemBox());
	default:
		break;
	}

	return false;
}

ObserverReference* TargetSelector::getObserverRefByTarget(WorldObject* target, TargetAction action) const
{
	auto const& lookupTable = m_actionTargetLookupTables[action];
	auto it = lookupTable.find(target->getData()->getGuid());
	if (it != lookupTable.end())
	{
		auto const& listIt = (*it).second;
		return *listIt;
	}

	return nullptr;
}

void TargetSelector::removeTargetsForAction(TargetAction action)
{
	auto& observerRefList = m_actionTargets[action];
	auto& lookupTable = m_actionTargetLookupTables[action];
	for (auto it = observerRefList.begin(); it != observerRefList.end(); )
	{
		ObserverReference* ref = *it;
		auto nRemoved = lookupTable.erase(ref->getTargetGuid());
		NS_ASSERT(nRemoved != 0);
		it = observerRefList.erase(it);

		ref->unlink();
		delete ref;
	}
}

void TargetSelector::removeTarget(TargetAction action, WorldObject* target)
{
	auto& observerRefList = m_actionTargets[action];
	auto& lookupTable = m_actionTargetLookupTables[action];
	auto it = lookupTable.find(target->getData()->getGuid());
	if (it != lookupTable.end())
	{
		auto const& listIt = (*it).second;
		observerRefList.erase(listIt);
		lookupTable.erase(it);
	}
}

void TargetSelector::removeAllTargets()
{
	for (auto i = 0; i < m_actionTargets.size(); ++i)
	{
		auto& observerRefList = m_actionTargets[i];
		for (auto refIt = observerRefList.begin(); refIt != observerRefList.end();)
		{
			ObserverReference* ref = *refIt;
			auto nRemoved = m_actionTargetLookupTables[i].erase(ref->getTargetGuid());
			NS_ASSERT(nRemoved != 0);
			refIt = observerRefList.erase(refIt);

			ref->unlink();
			delete ref;
		}
	}
}

ObserverReference* TargetSelector::selectNextObserverRef(TargetAction action)
{
	ObserverReference* currRef = nullptr;
	auto& observerRefList = m_actionTargets[action];
	if (!observerRefList.empty())
	{
		if (observerRefList.size() > 1)
			observerRefList.sort(TargetOrderPred());

		currRef = *observerRefList.begin();
		NS_ASSERT(currRef->isValid());
	}

	return currRef;
}

