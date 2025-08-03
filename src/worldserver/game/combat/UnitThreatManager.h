#ifndef __UNIT_THREAT_MANAGER_H__
#define __UNIT_THREAT_MANAGER_H__

#include "UnitHostileReference.h"

class Robot;

class UnitThreatManager
{
	typedef std::list<UnitHostileReference*> HostileRefList;
	typedef std::unordered_map<ObjectGuid, HostileRefList::iterator> HostileRefListIteratorMap;
public:
	UnitThreatManager(Robot* owner);
	~UnitThreatManager();

	Robot* getOwner() const { return m_owner; }

	Unit* getHostileTarget();

	void addThreat(Unit* victim, UnitThreatType type, float variant);
	bool isHostileTarget(Unit* victim) const;
	void updateAllThreats(UnitThreatType type, float variant = 0.f);
	std::list<UnitHostileReference*> const& getThreatList() const { return m_threatList; }
	void removeAllThreats();

	void setDirty(bool isDirty) { m_isDirty = isDirty; }

private:
	UnitHostileReference* selectNextHostileRef();
	void update();
	UnitHostileReference* getReferenceByTarget(Unit* target) const;

	Robot* m_owner;
	bool m_isDirty;
	HostileRefList m_threatList;
	HostileRefListIteratorMap m_threatLookupTable;
};


#endif // __UNIT_THREAT_MANAGER_H__

