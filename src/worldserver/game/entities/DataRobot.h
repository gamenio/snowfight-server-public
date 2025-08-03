#ifndef __DATA_ROBOT_H__
#define __DATA_ROBOT_H__

#include "Common.h"
#include "DataUnit.h"				
#include "game/world/Locale.h"

// 机器人的熟练等级范围
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
	uint8 combatGrade;		// 战斗等级
	uint32 templateId;
	uint8 minLevel;			// 最小等级
	uint8 maxLevel;			// 最大等级
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
	NATURE_EFFECT_ABANDON_HIDING_HEALTH_PERCENT						= 1,	// 放弃躲藏时的生命值比例
	NATURE_EFFECT_ABANDON_COLLECTING_HEALTH_PERCENT					= 2,	// 放弃收集时的生命值比例
	NATURE_EFFECT_ABANDON_ESCAPE_HEALTH_PERCENT						= 3,	// 放弃逃跑时的生命值比例
	NATURE_EFFECT_ABANDON_ESCAPE_TO_CHASE_HEALTH_PERCENT			= 4,	// 放弃逃跑开始追击时的生命值比例
	NATURE_EFFECT_USEITEM_ESCAPE_HEALTH_PERCENT						= 5,	// 使用物品逃跑时的生命值比例
	NATURE_EFFECT_USEITEM_FIRST_AID_HEALTH_PERCENT					= 6,	// 使用物品急救时的生命值比例
	NATURE_EFFECT_LOCK_ITEM_PICKUP_TIME_REMAINING					= 7,	// 锁定物品时的捡拾剩余时间。单位：毫秒
	NATURE_EFFECT_ABANDON_ESCAPE_IN_1V1_COMBAT_HEALTH_PERCENT		= 8,	// 一对一战斗中放弃逃跑的生命值比例
	NATURE_EFFECT_ABANDON_UNLOCKING_HEALTH_PERCENT					= 9,	// 放弃解锁时的生命值比例
	NATURE_EFFECT_CHASE_THREAT_DISTANCE_WEIGHTED_PERCENT			= 10,	// 追击时敌人距离的威胁权重比例
	NATURE_EFFECT_CHASE_THREAT_ENEMY_HEALTH_WEIGHTED_PERCENT		= 11,	// 追击时敌人生命值的威胁权重比例
	NATURE_EFFECT_CHASE_THREAT_RECEIVED_DAMAGE_WEIGHTED_PERCENT		= 12,	// 追击时敌人对我累计造成伤害的威胁权重比例
	NATURE_EFFECT_CHASE_THREAT_ENEMY_CHARGED_POWER_WEIGHTED_PERCENT	= 13,	// 追击时敌人蓄力攻击威力的威胁权重比例
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

	// 属性阶段
	void setStage(uint8 stage) { m_stage = stage; }
	uint8 getStage() const { return m_stage; }

	// 反应标记
	void clearReactFlag(uint32 flag) { if (hasReactFlag(flag)) m_reactFlags &= ~flag; }
	bool hasReactFlag(uint32 flag) const { return (m_reactFlags & flag) != 0; }
	void addReactFlag(uint32 flag) { if (!hasReactFlag(flag)) m_reactFlags |= flag; }
	void setReactFlags(uint32 flags) { m_reactFlags = flags; }
	uint32 getReactFlags() const { return m_reactFlags; }

	// 机器人的熟练等级
	void setProficiencyLevel(uint8 level) { m_proficiencyLevel = level; }
	uint8 getProficiencyLevel() const { return m_proficiencyLevel; }

	// 机器人性格
	void setNatureType(uint32 type) { m_natureType = type; }
	uint32 getNatureType() const { return m_natureType; }

	// AI动作类型
	void setAIActionType(uint32 actionType);
	uint32 getAIActionType() const { return m_aiActionType; }

	// AI动作状态
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
