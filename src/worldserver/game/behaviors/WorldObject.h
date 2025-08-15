#ifndef __WORLD_OBJECT_H__
#define __WORLD_OBJECT_H__

#include "game/entities/DataWorldObject.h"
#include "game/entities/updates/UpdateObject.h"
#include "game/behaviors/LocatorObject.h"
#include "game/maps/BattleMap.h"
#include "game/grids/GridAreaImpl.h"
#include "game/ai/ObserverRefManager.h"
#include "game/combat/ProjectileCollisionRefManager.h"
#include "Object.h"

// The maximum size of the object. All object sizes should be within this value
#define MAX_SIZE_OF_OBJECT		64.0f


enum NotifyFlag
{
	NOTIFY_NONE						= 0,
	NOTIFY_VISIBILITY_CHANGED		= 1 << 0, // Notify when the visibility, position, or AI state of the object changes
	NOTIFY_TRACEABILITY_CHANGED		= 1 << 1, // Notify when the traceability of the object changes
	NOTIFY_SAFETY_CHANGED			= 1 << 2, // Notify when the safety of the object changes
};

template<typename T>
class GridObject
{
public:
	GridObject()
	{ }

	~GridObject() { }

	bool isInGrid() const { return m_gridRef.isValid(); }

	void addToGrid(GridRefManager<T>& g)
	{ 
		NS_ASSERT(!isInGrid()); 
		m_gridRef.link(&g, static_cast<T*>(this));
	}

	void removeFromGrid() 
	{ 
		NS_ASSERT(isInGrid()); 
		m_gridRef.unlink();
	}

private:
	GridReference<T> m_gridRef;
};

class WorldObject: public Object
{
public:
	WorldObject();
	virtual ~WorldObject() = 0;

	void setMap(BattleMap* map);
	BattleMap* getMap() const { return m_map; }

	virtual void setLocator(LocatorObject* locator);
	virtual LocatorObject* getLocator() const { return m_locator; }

	virtual void update(NSTime diff) { }

	virtual void removeFromWorld();
	virtual void cleanupBeforeDelete();

	void destroyForNearbyPlayers();
	void invisibleForNearbyPlayers();
	void buildValuesUpdate(PlayerUpdateMapType& updateMap) override;

	void setVisible(bool visible) { m_isVisible = visible; }
	bool isVisible() const { return m_isVisible; }
	virtual void sendInitialVisiblePacketsToPlayer(Player* player) {}
	// Update the visibility, position, or AI state of the object
	// When forced is true, force all nearby players to update the visibility of the object
	virtual void updateObjectVisibility(bool forced = false);
	// Update the safety of the object
	virtual void updateObjectSafety();

	// Update the traceability of the object
	// When forced is true, force all nearby players to update the traceability of the object
	virtual void updateObjectTraceability(bool forced = false);

	void addToNotify(uint16 f) { m_notifyflags |= f; }
	bool isNeedNotify(uint16 f) const { return (m_notifyflags & f) != 0; }
	void clearNotify(uint16 f) { if (isNeedNotify(f)) m_notifyflags &= ~f; }
	void resetAllNotifies() { m_notifyflags = 0; }

	Rect getBoundingBox() const;

	bool isWithinDist(Point const& pos, float dist) const;
	bool isWithinDist(WorldObject const* obj, float dist) const;

	template<typename VISITOR> void visitNearbyObjects(Size const& range, VISITOR& visitor) const { if (this->isInWorld()) this->getMap()->visitGrids(this->getData()->getPosition(), range, visitor); }
	template<typename VISITOR> void visitNearbyObjects(float radius, VISITOR& visitor) const { if (this->isInWorld()) this->getMap()->visitGrids(this->getData()->getPosition(), radius, visitor); }
	template<typename VISITOR> void visitAllObjects(VISITOR& visitor) const { if (this->isInWorld()) this->getMap()->visitCreatedGrids(visitor); }
	template<typename VISITOR> void visitNearbyObjectsInMaxVisibleRange(VISITOR& visitor) const { if (this->isInWorld()) this->getMap()->visitGridsInMaxVisibleRange(this->getData()->getPosition(), visitor);}

	bool addToObjectUpdate() override;
	void removeFromObjectUpdate() override;

	ObserverRefManager* getObserverRefManager() { return &m_observerRefManager; }
	virtual DataWorldObject* getData() const override { return static_cast<DataWorldObject*>(m_data); }

protected:
	BattleMap* m_map;
	uint16 m_notifyflags;

	bool m_isVisible;

	LocatorObject* m_locator;

	ObserverRefManager m_observerRefManager;
};

class AttackableObject: public WorldObject
{
public:
	AttackableObject() {}
	~AttackableObject() {}

	virtual void cleanupBeforeDelete() override;

	// The attacker of the object
	void addAttacker(Unit* attacker) { m_attackers.insert(attacker); }
	void removeAttacker(Unit* attacker) { m_attackers.erase(attacker); }
	std::unordered_set<Unit*> const& getAllAttacker() const { return m_attackers; }
	void removeAllAttackers();

	// Watcher of an object. If A is within B's line of sight (unobstructed), 
	// then B may become A's watcher. For example, an attacker is a type of watcher
	void addWatcher(Unit* watcher) { if (m_watchers.find(watcher) == m_watchers.end()) m_watchers.insert(watcher); }
	void removeWatcher(Unit* watcher) { if (m_watchers.find(watcher) != m_watchers.end()) m_watchers.erase(watcher); }
	bool hasWatcher(Unit* watcher) const { return m_watchers.find(watcher) != m_watchers.end(); }
	void removeAllWatchers() { m_watchers.clear(); }

	// Called when the object begins to collide with the projectile
	// The value range of precision is 0-1.0. The higher the value, the higher the precision
	virtual void enterCollision(Projectile* proj, float precision) {}

	ProjectileCollisionRefManager* getProjectileCollisionRefManager() { return &m_projCollisionRefManager; }

protected:
	std::unordered_set<Unit*> m_attackers;
	std::unordered_set<Unit*> m_watchers;

	ProjectileCollisionRefManager m_projCollisionRefManager;
};

#endif // __WORLD_OBJECT_H__

