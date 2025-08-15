#ifndef __OBSERVER_REFERENCE_H__
#define __OBSERVER_REFERENCE_H__

#include "game/dynamic/reference/Reference.h"
#include "game/behaviors/UnitHelper.h"
#include "game/entities/ObjectGuid.h"

// Target execution actions
enum TargetAction
{
	TARGET_ACTION_COLLECT,
	TARGET_ACTION_COMBAT,
	TARGET_ACTION_UNLOCK,
	MAX_TARGET_ACTIONS,
};

class WorldObject;
class TargetSelector;

class ObserverReference: public Reference<WorldObject, TargetSelector>
{
public:
	ObserverReference(WorldObject* object, TargetAction action, TargetSelector* targetManager);
	~ObserverReference();

	ObjectGuid getTargetGuid() const { return m_targetGuid; }
	TargetAction getTargetAction() const { return m_action; }

	void update();
	float getOrder() const;

protected:
	void buildLink() override;
	void destroyLink() override;

private:
	ObjectGuid m_targetGuid;
	TargetAction m_action;
	float m_weight;
};

#endif // __OBSERVER_REFERENCE_H__