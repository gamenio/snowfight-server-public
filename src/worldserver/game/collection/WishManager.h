#ifndef __WISH_MANAGER_H__
#define __WISH_MANAGER_H__

#include "InterestedReference.h"

class Robot;

class WishManager
{
	typedef std::list<InterestedReference*> InterestedRefList;
	typedef std::unordered_map<ObjectGuid, InterestedRefList::iterator> InterestedRefListIteratorMap;
public:
	WishManager(Robot* owner);
	~WishManager();

	Robot* getOwner() const { return m_owner; }

	Item* getInterestedTarget();

	void addWish(Item* item);
	bool isInterestedTarget(Item* item) const;
	std::list<InterestedReference*> const& getWishList() const { return m_wishList; }
	void removeAllWishes();

	void setDirty(bool isDirty) { m_isDirty = isDirty; }

private:
	InterestedReference* selectNextInterestedRef();
	void update();
	InterestedReference* getReferenceByTarget(Item* target) const;

	Robot* m_owner;
	bool m_isDirty;
	InterestedRefList m_wishList;
	InterestedRefListIteratorMap m_wishLookupTable;
};


#endif // __WISH_MANAGER_H__

