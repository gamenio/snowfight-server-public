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

// 对象的最大尺寸，所有对象尺寸应该在这个值之内
#define MAX_SIZE_OF_OBJECT		64.0f


enum NotifyFlag
{
	NOTIFY_NONE						= 0,
	NOTIFY_VISIBILITY_CHANGED		= 1 << 0, // 通知对象的可见性、位置或AI状态发生改变
	NOTIFY_TRACEABILITY_CHANGED		= 1 << 1, // 通知对象的可追踪性发生改变
	NOTIFY_SAFETY_CHANGED			= 1 << 2, // 通知对象的安全性发生改变
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
	// 更新对象的可见性、位置或AI状态
	// 当forced为true时，强制为附近所有玩家更新对象的可见性
	virtual void updateObjectVisibility(bool forced = false);
	// 更新对象的安全性
	virtual void updateObjectSafety();

	// 更新对象的可追踪性
	// 当forced为true时，强制为附近所有玩家更新对象的可追踪性
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

	// 对象的攻击者
	void addAttacker(Unit* attacker) { m_attackers.insert(attacker); }
	void removeAttacker(Unit* attacker) { m_attackers.erase(attacker); }
	std::unordered_set<Unit*> const& getAllAttacker() const { return m_attackers; }
	void removeAllAttackers();

	// 对象的注视者。如果A在B的视线内(无遮挡)，则B将有可能成为A注视者，例如：攻击者便是注视者的一种
	void addWatcher(Unit* watcher) { if (m_watchers.find(watcher) == m_watchers.end()) m_watchers.insert(watcher); }
	void removeWatcher(Unit* watcher) { if (m_watchers.find(watcher) != m_watchers.end()) m_watchers.erase(watcher); }
	bool hasWatcher(Unit* watcher) const { return m_watchers.find(watcher) != m_watchers.end(); }
	void removeAllWatchers() { m_watchers.clear(); }

	// 当对象开始接触抛射体时调用
	// precision的取值范围为0-1.0，值越大精度越高
	virtual void enterCollision(Projectile* proj, float precision) {}

	ProjectileCollisionRefManager* getProjectileCollisionRefManager() { return &m_projCollisionRefManager; }

protected:
	std::unordered_set<Unit*> m_attackers;
	std::unordered_set<Unit*> m_watchers;

	ProjectileCollisionRefManager m_projCollisionRefManager;
};

#endif // __WORLD_OBJECT_H__

