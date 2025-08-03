#ifndef __LOCATOR_OBJECT_H__
#define __LOCATOR_OBJECT_H__

#include "game/maps/BattleMap.h"
#include "game/entities/DataLocatorObject.h"
#include "Object.h"

class LocatorObject : public Object
{
public:
	LocatorObject();
    virtual ~LocatorObject();

	void setOwner(WorldObject* object) { m_owner = object; }
	WorldObject* getOwner() const { return m_owner; }

	virtual void removeFromWorld() override;

	void destroyForNearbyPlayers();
	void invisibleForNearbyPlayers();
	void buildValuesUpdate(PlayerUpdateMapType& updateMap) override;

	bool addToObjectUpdate() override;
	void removeFromObjectUpdate() override;

	virtual DataLocatorObject* getData() const override { return static_cast<DataLocatorObject*>(m_data); }

private:
	WorldObject* m_owner;
};


#endif // __LOCATOR_OBJECT_H__
