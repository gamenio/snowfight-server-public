#ifndef __TARGET_MANAGER_H__
#define __TARGET_MANAGER_H__

#include "game/behaviors/UnitHelper.h"
#include "game/entities/ObjectGuid.h"
#include "ObserverReference.h"

class Robot;

class TargetSelector
{
	typedef std::list<ObserverReference*> ObserverRefList;
	typedef std::unordered_map<ObjectGuid, ObserverRefList::iterator> ObserverRefListIteratorMap;
public:
	TargetSelector(Robot* owner);
	~TargetSelector();

	Robot* getOwner() const { return m_owner; }

	WorldObject* getVaildTarget(TargetAction action);

	void addTarget(WorldObject* target, TargetAction action);
	std::list<ObserverReference*> const& getTargetList(TargetAction action) const { return m_actionTargets[action]; }

	void removeTarget(TargetAction action, WorldObject* target);
	void removeTargetsForAction(TargetAction action);
	void removeAllTargets();

private:
	ObserverReference* selectNextObserverRef(TargetAction action);
	void update(TargetAction action);
	bool canBeTarget(ObserverReference* targetRef);
	ObserverReference* getObserverRefByTarget(WorldObject* target, TargetAction action) const;

	Robot* m_owner;
	std::array<ObserverRefList, MAX_TARGET_ACTIONS>  m_actionTargets;
	std::array<ObserverRefListIteratorMap, MAX_TARGET_ACTIONS> m_actionTargetLookupTables;

};


#endif // __TARGET_MANAGER_H__

