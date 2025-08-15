#ifndef __DATA_PLAYER_H__
#define __DATA_PLAYER_H__

#include "game/world/Locale.h"
#include "DataUnit.h"

// Game controller type
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
	uint8 originLevel;			// Origin level
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

	// Is it GM
	void setGM(bool isGM);
	bool isGM() const { return m_isGM; }

	// Client language
	void setLang(LangType lang) { m_lang = lang; }
	LangType getLang() const { return m_lang; }

	// Game controller type
	void setControllerType(ControllerType controllerType) { m_controllerType = controllerType; }
	ControllerType getControllerType() const { return m_controllerType; }

	// Is it a new player
	bool isNewPlayer() const { return m_isNewPlayer; }
	void setNewPlayer(bool isNewPlayer) { m_isNewPlayer = isNewPlayer; }

	// Is it a training ground trainee
	bool isTrainee() const { return m_isTrainee; }
	void setTrainee(bool isTrainee) { m_isTrainee = isTrainee; }

	// Free golds reward stage
	void setRewardStage(uint8 stage) { m_rewardStage = stage; }
	uint8 getRewardStage() const { return  m_rewardStage; }

	// Cumulative days of daily reward
	void setDailyRewardDays(int32 days) { m_dailyRewardDays = days; }
	int32 getDailyRewardDays() const { return m_dailyRewardDays; }

	// The player's combat levelc
	void setCombatGrade(uint8 grade) { m_combatGrade = grade; }
	uint8 getCombatGrade() const { return m_combatGrade; }

	void updateDataForPlayer(UpdateType updateType, Player* player) override;

	void setKillCount(int32 count) override;

	// Item in the slot
	ObjectGuid const& getItem(int32 slot) const { return m_items[slot]; }
	void setItem(int32 slot, ObjectGuid const& guid);

	void setConcealmentState(ConcealmentState state) override;
	void setPickupDuration(int32 duration) override;

	// The target of picking up
	ObjectGuid const& getPickupTarget() const { return m_pickupTarget; }
	void setPickupTarget(ObjectGuid const& guid);

	void setExperience(int32 xp) override;
	void setNextLevelXP(int32 xp) override;

	void setMoney(int32 money) override;

	void setProperty(int32 property) { m_property = property; }
	int32 getProperty() const { return m_property; }

	// Statistic stage
	void setStatStage(StatType statType, uint8 stage) { m_statStageList[statType] = stage; }
	uint8 getStatStage(StatType statType) const { return m_statStageList[statType]; }

	// Selected map ID
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