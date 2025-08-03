#ifndef __MOTION_MASTER_H__
#define __MOTION_MASTER_H__

#include "Common.h"
#include "game/tiles/TileCoord.h"


enum MotionSlot
{
	MOTION_SLOT_IDLE,
	MOTION_SLOT_ACTIVE,
	MAX_MOTION_SLOT
};

class WorldObject;
class Unit;
class Robot;
class Item;
class ItemBox;
class MovementGenerator;

class MotionMaster
{
public:
	MotionMaster(Robot* owner);
	~MotionMaster();

	MovementGenerator* pop();
	MovementGenerator* top();
	MovementGenerator* getMotion(MotionSlot slot) const { return m_motions[slot]; }
	int32 size() const { return m_top + 1; }
	bool empty() const { return m_top < 0; }
	void clear();
	void remove(MotionSlot slot);
	void reset(MotionSlot slot);

	void updateMotion(NSTime diff);

	void moveIdle();
	void moveExplore();
	void moveChase(Unit* target, bool isSmart);
	void moveChase(ItemBox* target, bool isSmart);
	void moveEscape(Unit* target);
	void movePoint(TileCoord const& point, bool isBypassEnemy = false);
	void moveSeek(TileCoord const& hidingSpot);

private:
	void mutate(MovementGenerator* m, MotionSlot slot);
	void directDelete(MovementGenerator* m);

	MovementGenerator* m_motions[MAX_MOTION_SLOT];
	int32 m_top;
	Robot* m_owner;
};

#endif // __MOTION_MASTER_H__