#include "WishManager.h"

#include "game/behaviors/Robot.h"


class WishOrderPred
{
public:
	WishOrderPred(bool ascending = false) : m_ascending(ascending) { }
	bool operator() (InterestedReference const* a, InterestedReference const* b) const
	{
		return m_ascending ? a->getWish() < b->getWish() : a->getWish() > b->getWish();
	}
private:
	const bool m_ascending;
};

WishManager::WishManager(Robot* owner):
	m_owner(owner),
	m_isDirty(false)
{
}


WishManager::~WishManager()
{
	m_owner = nullptr;
	this->removeAllWishes();
}


Item* WishManager::getInterestedTarget()
{
	this->update();

	if (InterestedReference* ref = this->selectNextInterestedRef())
		return ref->getTarget();

	return nullptr;
}

void WishManager::addWish(Item* item)
{
	InterestedReference* interestedRef = this->getReferenceByTarget(item);
	if (!interestedRef)
	{
		interestedRef = new InterestedReference(item, this);
		auto it = m_wishList.emplace(m_wishList.end(), interestedRef);
		NS_ASSERT(it != m_wishList.end());
		auto ret = m_wishLookupTable.emplace(item->getData()->getGuid(), it);
		NS_ASSERT(ret.second);
		m_isDirty = true;
	}
}

bool WishManager::isInterestedTarget(Item* item) const
{
	auto it = m_wishLookupTable.find(item->getData()->getGuid());
	return it != m_wishLookupTable.end();
}

void WishManager::update()
{ 
	if (m_isDirty)
	{
		for (auto it = m_wishList.begin(); it != m_wishList.end(); )
		{
			InterestedReference* interestedRef = *it;
			if (!interestedRef->isValid())
			{
				it = m_wishList.erase(it);
				auto nRemoved = m_wishLookupTable.erase(interestedRef->getTargetGuid());
				NS_ASSERT(nRemoved != 0);

				delete interestedRef;
			}
			else
			{
				interestedRef->update();
				++it;
			}
		}

		if (m_wishList.size() > 1)
			m_wishList.sort(WishOrderPred());
	}

	m_isDirty = false;
}

InterestedReference* WishManager::getReferenceByTarget(Item* target) const
{
	auto it = m_wishLookupTable.find(target->getData()->getGuid());
	if (it != m_wishLookupTable.end())
	{
		auto const& listIt = (*it).second;
		return *listIt;
	}

	return nullptr;
}

void WishManager::removeAllWishes()
{
	for (auto it = m_wishList.begin(); it != m_wishList.end(); )
	{
		InterestedReference* interestedRef = *it;
		it = m_wishList.erase(it);
		auto nRemoved = m_wishLookupTable.erase(interestedRef->getTargetGuid());
		NS_ASSERT(nRemoved != 0);

		interestedRef->unlink();
		delete interestedRef;
	}
}

InterestedReference* WishManager::selectNextInterestedRef()
{
	InterestedReference* currRef = nullptr;
	for (auto it = m_wishList.begin(); it != m_wishList.end(); ++it)
	{
		InterestedReference* ref = *it;
		NS_ASSERT(ref->isValid());
		Item* item = ref->getTarget();
		if (m_owner->canCollect(item))
		{
			currRef = ref;
			break;
		}

	}
	return currRef;
}

