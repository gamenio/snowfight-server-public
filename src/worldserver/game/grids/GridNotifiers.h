#ifndef __GRID_NOTIFIERS_H__
#define __GRID_NOTIFIERS_H__

#include "game/behaviors/Player.h"
#include "game/behaviors/Robot.h"
#include "game/behaviors/ItemBox.h"
#include "game/behaviors/Item.h"
#include "game/behaviors/Projectile.h"
#include "game/maps/BattleMap.h"
#include "GridDefines.h"


class TrackingNotifier
{
public:
	TrackingNotifier(Player& player) :
		m_player(player)
	{
	}

	template<typename T>
	void updateTraceability(GridRefManager<T>& g)
	{
		for (auto it = g.begin(); it != g.end(); ++it)
		{
			T* obj = it->getSource();
			m_player.updateTraceabilityOf(obj, m_updateObject);
		}
	}

	template<typename T> void visit(GridRefManager<T>& g) { }
	void visit(PlayerGridType& g) { this->updateTraceability(g); }
	void visit(RobotGridType& g) { this->updateTraceability(g); }

	void sendToSelf();

protected:
	Player& m_player;
	UpdateObject m_updateObject;
};

class TrackingChangesNotifier
{
public:
	TrackingChangesNotifier(WorldObject& object) :
		m_object(object)
	{
	}

	template<class T> void visit(GridRefManager<T>&) { }
	void visit(PlayerGridType& g);

private:
	WorldObject& m_object;
};

class PlayerTrackingNotifier : public TrackingNotifier
{
public:
	PlayerTrackingNotifier(Player& player) :
		TrackingNotifier(player) {}

	template<class T> void visit(GridRefManager<T>&) { }
	void visit(PlayerGridType& g);
	void visit(RobotGridType& g);
};

class RobotTrackingNotifier
{
public:
	RobotTrackingNotifier(Robot& robot) :
		m_robot(robot) {}

	template<class T> void visit(GridRefManager<T>&) { }
	void visit(PlayerGridType& g);

private:
	Robot& m_robot;
};

class VisibleNotifier
{
public:
	VisibleNotifier(Player& player) : 
		m_player(player)
	{ 
	}

	template<typename T>
	void visit(GridRefManager<T>& g)
	{
		for (auto it = g.begin(); it != g.end(); ++it)
			m_player.updateVisibilityOf(it->getSource(), m_updateObject, m_visibleNow);
	}

	void sendToSelf();

protected:
	Player& m_player;
	UpdateObject m_updateObject;
	std::unordered_set<WorldObject*> m_visibleNow;
};

class VisibleChangesNotifier
{
public:
	VisibleChangesNotifier(WorldObject& object) : 
		m_object(object)
	{

	}

	template<class T> void visit(GridRefManager<T>&) { }
	void visit(PlayerGridType& g);

private:
	WorldObject& m_object;
};

class RobotRelocationNotifier
{
public:
	RobotRelocationNotifier(Robot& robot):
		m_robot(robot){}

	void visit(PlayerGridType& g);
	void visit(RobotGridType& g);
	void visit(ItemBoxGridType& g);
	void visit(ItemGridType& g);
	void visit(ProjectileGridType& g);

private:
	Robot& m_robot;
};

class PlayerRelocationNotifier: public VisibleNotifier
{
public:
	PlayerRelocationNotifier(Player& player):
			VisibleNotifier(player){}

	void visit(PlayerGridType& g);
	void visit(RobotGridType& g);
	void visit(ItemBoxGridType& g);
	void visit(ItemGridType& g);
	void visit(ProjectileGridType& g);
};

class ItemBoxRelocationNotifier
{
public:
	ItemBoxRelocationNotifier(ItemBox& itemBox) :
		m_itemBox(itemBox) {}

	template<class T> void visit(GridRefManager<T>&) { }
	void visit(PlayerGridType& g);
	void visit(RobotGridType& g);

private:
	ItemBox& m_itemBox;
};

class ItemRelocationNotifier
{
public:
	ItemRelocationNotifier(Item& item) :
		m_item(item) {}

	template<class T> void visit(GridRefManager<T>&) { }
	void visit(PlayerGridType& g);
	void visit(RobotGridType& g);

private:
	Item& m_item;
};

class ProjectileRelocationNotifier
{
public:
	ProjectileRelocationNotifier(Projectile& proj) :
		m_proj(proj) {}

	template<class T> void visit(GridRefManager<T>&) { }
	void visit(PlayerGridType& g);
	void visit(RobotGridType& g);
	void visit(ItemBoxGridType& g);

private:
	Projectile& m_proj;
};

class ObjectRelocation
{
public:
	ObjectRelocation(BattleMap& map) :
		m_map(map)
	{
	}

	void visit(PlayerGridType& g);
	void visit(RobotGridType& g);
	void visit(ItemBoxGridType& g);
	void visit(ItemGridType& g);
	void visit(ProjectileGridType& g);

private:
	BattleMap& m_map;
};

class SafeZoneRelocationNotifier
{
public:
	SafeZoneRelocationNotifier(BattleMap& map) :
		m_map(map)
	{
	}

	template<class T> void visit(GridRefManager<T>&) { }
	void visit(PlayerGridType& g);
	void visit(RobotGridType& g);

private:
	BattleMap& m_map;
};

class ObjectUpdater
{
public:
	ObjectUpdater(NSTime diff) :
		m_diff(diff) {}

	template<typename T>
	void update(GridRefManager<T>& g)
	{
		for (auto it = g.begin(); it != g.end(); ++it)
			it->getSource()->update(m_diff);
	}

	template<typename T> void visit(GridRefManager<T>& g) {}

	void visit(ItemBoxGridType& g) { this->update(g); }
	void visit(ItemGridType& g) { this->update(g); }

private:
	NSTime m_diff;
};

class ObjectUpdateNotifier
{
public:
	ObjectUpdateNotifier(BattleMap& map, NSTime diff) :
		m_map(map),
		m_diff(diff),
		m_updater(diff),
		m_updaterVisitor(m_updater)
	{
	}

	template<class T> void visit(GridRefManager<T>&) { }
	void visit(RobotGridType& g);
	void visit(ProjectileGridType& g);

private:
	BattleMap& m_map;
	NSTime m_diff;
	ObjectUpdater m_updater;
	TypeContainerVisitor<ObjectUpdater, WorldTypeGridContainer> m_updaterVisitor;
};


class ObjectNotifyCleaner
{
public:

	template<typename T>
	void cleanNotify(GridRefManager<T>& g)
	{
		for (auto it = g.begin(); it != g.end(); ++it)
		{
			T* obj = it->getSource();
			obj->resetAllNotifies();
		}
	}

	void visit(PlayerGridType& g) { this->cleanNotify(g); }
	void visit(RobotGridType& g) { this->cleanNotify(g); }
	void visit(ItemBoxGridType& g) { this->cleanNotify(g); }
	void visit(ItemGridType& g) { this->cleanNotify(g); }
	void visit(ProjectileGridType& g) { this->cleanNotify(g); }
};

class ObjectDistrictCounterCleaner
{
public:
	ObjectDistrictCounterCleaner(uint32 districtId): 
		m_districtId(districtId) {}

	template<typename T>
	void cleanDistrictCounter(GridRefManager<T>& g)
	{
		for (auto it = g.begin(); it != g.end(); ++it)
		{
			T* obj = it->getSource();
			obj->removeDistrictCounter(m_districtId);
		}
	}

	template<class T> void visit(GridRefManager<T>&) { }
	void visit(RobotGridType& g) { this->cleanDistrictCounter(g); }

private:
	uint32 m_districtId;
};

#endif // __GRID_NOTIFIERS_H__

