#ifndef __DATA_ROBOT_H__
#define __DATA_ROBOT_H__

#include "Common.h"
#include "DataUnit.h"				
#include "game/world/Locale.h"

// The range of proficiency levels for the robot
#define PROFICIENCY_LEVEL_MIN			1
#define PROFICIENCY_LEVEL_MAX			6

enum ReactFlag
{
	REACT_FLAG_NONE						= 0,
	REACT_FLAG_PASSIVE_AGGRESSIVE		= 1 << 0,
};

struct RobotStats: UnitStats
{
	uint16 combatPower;
};

struct RobotTemplate
{
	uint32 id;
	uint32 displayId;
	std::vector<RobotStats> stageStatsList;
	std::multimap<uint16/* CombatPower */, uint8 /* Stage */, std::greater<uint16>> combatPowerStages;

	RobotStats const& getStageStats(uint8 stage) const
	{
		return stageStatsList[stage];
	}

	uint8 findStageByCombatPower(uint16 expectedCombatPower, uint16 minCombatPower) const;
};

struct RobotSpawnInfo
{
	uint8 combatGrade;		// Combat level
	uint32 templateId;
	uint8 minLevel;			// Minimum level
	uint8 maxLevel;			// Maximum level
};

struct RobotRegion
{
	std::string country;
	LangType lang;
};

struct RobotProficiency
{
	NSTime minDodgeReactionTime;
	NSTime maxDodgeReactionTime;
	float effectiveDodgeChance;
	NSTime minTargetReactionTime;
	NSTime maxTargetReactionTime;
	float aimingAccuracy;
	NSTime minContinuousAttackDelay;
	NSTime maxContinuousAttackDelay;
};

struct RobotDifficulty
{
	uint8 combatGrade;
	uint8 proficiencyLevel;
	uint32 natureType;
	uint16 maxCombatPower;
};

enum NatureType
{
	NATURE_CAREFUL			= 1,
	NATURE_CALM				= 2,
	NATURE_RASH				= 3,
};

enum NatureEffectType
{
	NATURE_EFFECT_ABANDON_HIDING_HEALTH_PERCENT						= 1,	// Abandon the percentage of health when hiding
	NATURE_EFFECT_ABANDON_COLLECTING_HEALTH_PERCENT					= 2,	// Abandon the percentage of health when collecting
	NATURE_EFFECT_ABANDON_ESCAPE_HEALTH_PERCENT						= 3,	// Abandon the percentage of health when escaping
	NATURE_EFFECT_ABANDON_ESCAPE_TO_CHASE_HEALTH_PERCENT			= 4,	// Abandon the percentage of health when starting to chase after escaping
	NATURE_EFFECT_USEITEM_ESCAPE_HEALTH_PERCENT						= 5,	// Percentage of health when using item to escape
	NATURE_EFFECT_USEITEM_FIRST_AID_HEALTH_PERCENT					= 6,	// Percentage of health when using item for first aid
	NATURE_EFFECT_LOCK_ITEM_PICKUP_TIME_REMAINING					= 7,	// Remaining pickup time when locking item. Unit: milliseconds
	NATURE_EFFECT_ABANDON_ESCAPE_IN_1V1_COMBAT_HEALTH_PERCENT		= 8,	// Abandon the percentage of health when escaping in one-on-one combat
	NATURE_EFFECT_ABANDON_UNLOCKING_HEALTH_PERCENT					= 9,	// Abandon the percentage of health when unlocking
	NATURE_EFFECT_CHASE_THREAT_DISTANCE_WEIGHTED_PERCENT			= 10,	// Threat weight percentage of enemy distance during chase
	NATURE_EFFECT_CHASE_THREAT_ENEMY_HEALTH_WEIGHTED_PERCENT		= 11,	// Threat weight percentage of enemy health during chase
	NATURE_EFFECT_CHASE_THREAT_RECEIVED_DAMAGE_WEIGHTED_PERCENT		= 12,	// Threat weight percentage of cumulative damage received from enemy during chase
	NATURE_EFFECT_CHASE_THREAT_ENEMY_CHARGED_POWER_WEIGHTED_PERCENT	= 13,	// Threat weight percentage of enemy charged attack power during chase
};

struct RobotNature
{
	int32 getEffectValue(NatureEffectType type) const
	{
		auto it = effects.find(type);
		if (it != effects.end())
			return (*it).second;

		return 0;
	}

	std::unordered_map<uint32 /* EffectType */, int32 /* EffectValue */> effects;
};

class DataRobot: public DataUnit
{
public:
	DataRobot();
	virtual ~DataRobot();

	void writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const override;

	void updateDataForPlayer(UpdateType updateType, Player* player) override;

	void setTarget(ObjectGuid const& guid);
	ObjectGuid const& getTarget() const { return m_target; }

	// Statistics stage
	void setStage(uint8 stage) { m_stage = stage; }
	uint8 getStage() const { return m_stage; }

	// Reaction flag
	void clearReactFlag(uint32 flag) { if (hasReactFlag(flag)) m_reactFlags &= ~flag; }
	bool hasReactFlag(uint32 flag) const { return (m_reactFlags & flag) != 0; }
	void addReactFlag(uint32 flag) { if (!hasReactFlag(flag)) m_reactFlags |= flag; }
	void setReactFlags(uint32 flags) { m_reactFlags = flags; }
	uint32 getReactFlags() const { return m_reactFlags; }

	// Robot proficiency level
	void setProficiencyLevel(uint8 level) { m_proficiencyLevel = level; }
	uint8 getProficiencyLevel() const { return m_proficiencyLevel; }

	// Robot nature type
	void setNatureType(uint32 type) { m_natureType = type; }
	uint32 getNatureType() const { return m_natureType; }

	// AI action type
	void setAIActionType(uint32 actionType);
	uint32 getAIActionType() const { return m_aiActionType; }

	// AI action state
	void setAIActionState(uint32 actionState);
	uint32 getAIActionState() const { return m_aiActionState; }

private:
	ObjectGuid m_target;
	uint8 m_stage;
	uint32 m_reactFlags;
	uint8 m_proficiencyLevel;
	uint32 m_natureType;

	uint32 m_aiActionType;
	uint32 m_aiActionState;
};



#endif // __DATA_ROBOT_H__
