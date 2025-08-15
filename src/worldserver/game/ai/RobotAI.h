#ifndef __ROBOT_AI_H__
#define __ROBOT_AI_H__

#include "game/tiles/TileCoord.h"
#include "UnitAI.h"

class Unit;
class Robot;
class ItemBox;
class Item;
class Projectile;

enum RobotAIType
{
	ROBOTAI_TYPE_NONE,
	ROBOTAI_TYPE_SPARRING,
	ROBOTAI_TYPE_TRAINING,
};

// AI action type.
// Sorted by execution order, with higher values executed first
enum AIActionType
{
	AI_ACTION_TYPE_NONE,
	AI_ACTION_TYPE_EXPLORE,
	AI_ACTION_TYPE_SEEK,
	AI_ACTION_TYPE_UNLOCK,
	AI_ACTION_TYPE_COLLECT,
	AI_ACTION_TYPE_COMBAT,
	AI_ACTION_TYPE_UNLOCK_DIRECTLY,
	AI_ACTION_TYPE_HIDE,
	AI_ACTION_TYPE_COLLECT_DIRECTLY,
	MAX_AI_ACTION_TYPES,
};

class RobotAI : public UnitAI
{
public:
	RobotAI(Robot* robot);
	~RobotAI();

	virtual RobotAIType getType() const = 0;
	void resetAI() override;

	virtual void combatDelayed(Unit* victim) {}
	virtual void combatStart(Unit* victim) {}

	virtual void moveInLineOfSight(Unit* who) {}
	virtual void moveInLineOfSight(ItemBox* itemBox) {}
	virtual void moveInLineOfSight(Item* item) {}
	virtual void moveInLineOfSight(Projectile* proj) {}
	virtual void discoverHidingSpot(TileCoord const& tileCoord) {}

	virtual void sayOpeningSmileyIfNeeded() {}

	AIActionType getAIAction() const { return m_action; }
	void setAIAction(AIActionType action);
	virtual void clearAIAction(AIActionType action);

protected:
	Robot* m_me;
	AIActionType m_action;
};

#endif // __ROBOT_AI_H__

