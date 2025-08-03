#ifndef __DATA_PLAYER_H__
#define __DATA_PLAYER_H__

#include "game/world/Locale.h"
#include "DataUnit.h"

// 游戏控制器类型
enum ControllerType
{
	CONTROLLER_TYPE_UNKNOWN,
	CONTROLLER_TYPE_TAP,
	CONTROLLER_TYPE_JOYSTICK,
};

struct PlayerTemplate
{
	uint32 id;
	uint32 displayId;
	uint8 originLevel;			// 初始等级
	std::vector<UnitStats> stageStatsList;

	UnitStats const& getStageStats(uint8 stage) const
	{
		return stageStatsList[stage];
	}
};

struct RewardForRanking
{
	int32 baseMoney;
	int32 maxMoney;
	int32 moneyRewardedPlayersPercent;
	int32 baseXP;
	int32 maxXP;
	int32 xpRewardedPlayersPercent;

	int32 getMoney(int32 rankNo, int32 rankTotal) const;
	int32 getXP(int32 rankNo, int32 rankTotal) const;
};

class DataPlayer : public DataUnit
{
public:
	DataPlayer();
	virtual ~DataPlayer();

	void writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const override;

	void setViewport(Size const& size);
	Size const& getViewport() const { return m_viewport; }
	Size const& getVisibleRange() const { return m_visibleRange; }

	// 是否为GM
	void setGM(bool isGM);
	bool isGM() const { return m_isGM; }

	// 客户端语言
	void setLang(LangType lang) { m_lang = lang; }
	LangType getLang() const { return m_lang; }

	// 游戏控制器类型
	void setControllerType(ControllerType controllerType) { m_controllerType = controllerType; }
	ControllerType getControllerType() const { return m_controllerType; }

	// 是否为新玩家
	bool isNewPlayer() const { return m_isNewPlayer; }
	void setNewPlayer(bool isNewPlayer) { m_isNewPlayer = isNewPlayer; }

	// 是否为训练场学员
	bool isTrainee() const { return m_isTrainee; }
	void setTrainee(bool isTrainee) { m_isTrainee = isTrainee; }

	// 免费金币奖励阶段
	void setRewardStage(uint8 stage) { m_rewardStage = stage; }
	uint8 getRewardStage() const { return  m_rewardStage; }

	// 每日奖励累计天数
	void setDailyRewardDays(int32 days) { m_dailyRewardDays = days; }
	int32 getDailyRewardDays() const { return m_dailyRewardDays; }

	// 玩家的战斗等级
	void setCombatGrade(uint8 grade) { m_combatGrade = grade; }
	uint8 getCombatGrade() const { return m_combatGrade; }

	void updateDataForPlayer(UpdateType updateType, Player* player) override;

	void setKillCount(int32 count) override;

	// 槽中的物品
	ObjectGuid const& getItem(int32 slot) const { return m_items[slot]; }
	void setItem(int32 slot, ObjectGuid const& guid);

	void setConcealmentState(ConcealmentState state) override;
	void setPickupDuration(int32 duration) override;

	// 捡拾的目标
	ObjectGuid const& getPickupTarget() const { return m_pickupTarget; }
	void setPickupTarget(ObjectGuid const& guid);

	void setExperience(int32 xp) override;
	void setNextLevelXP(int32 xp) override;

	void setMoney(int32 money) override;

	void setProperty(int32 property) { m_property = property; }
	int32 getProperty() const { return m_property; }

	// 属性阶段
	void setStatStage(StatType statType, uint8 stage) { m_statStageList[statType] = stage; }
	uint8 getStatStage(StatType statType) const { return m_statStageList[statType]; }

	// 选定的地图ID
	void setSelectedMapId(uint16 mapId) { m_selectedMapId = mapId; }
	uint16 getSelectedMapId() const { return m_selectedMapId; }

protected:
	bool m_isGM;
	std::array<ObjectGuid, UNIT_SLOTS_COUNT> m_items;
	ObjectGuid m_pickupTarget;

	Size m_viewport;
	Size m_visibleRange;
	LangType m_lang;
	ControllerType m_controllerType;

	bool m_isNewPlayer;
	bool m_isTrainee;
	uint8 m_rewardStage;
	int32 m_dailyRewardDays;
	uint8 m_combatGrade;
	int32 m_property;
	std::array<uint8, MAX_STAT_TYPES> m_statStageList;
	uint16 m_selectedMapId;
};

#endif // __DATA_PLAYER_H__