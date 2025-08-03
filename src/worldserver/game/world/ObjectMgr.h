#ifndef __OBJECT_MGR_H__
#define __OBJECT_MGR_H__

#include "SQLiteCpp/SQLiteCpp.h"

#include "Common.h"
#include "game/entities/DataPlayer.h"
#include "game/entities/DataRobot.h"
#include "game/entities/DataItemBox.h"
#include "game/entities/DataItem.h"

typedef std::vector<ObjectDistribution> ObjectDistributionList;
typedef std::unordered_map<uint16 /* MapId */, ObjectDistributionList> MapObjectDistributionListContainer;

typedef std::vector<CombatGrade> CombatGradeList;
typedef std::unordered_map<uint32/* ItemClass */, std::unordered_map<uint32/* ItemSubclass */, UnitLootItemTemplate>> UnitLootItemTemplateContainer;

typedef std::unordered_map<uint32/* TemplateId */, PlayerTemplate> PlayerTemplateContainer;
typedef std::unordered_map<uint32 /* LangType */, std::string> PlayerNameContainer;

typedef std::unordered_map<uint32/* TemplateId */, RobotTemplate> RobotTemplateContainer;
typedef std::vector<RobotSpawnInfo> RobotSpawnList;
typedef std::unordered_map<uint8 /* CombatGrade */, RobotSpawnList> GradedRobotSpawnListContainer;
typedef std::unordered_map<uint8 /* CombatGrade */, std::vector<double>>  GradedRobotSpawnWeightListContainer;
typedef std::vector<std::string> RobotNameList;
typedef std::unordered_map<uint32 /* LangType */, RobotNameList> RobotNameListContainer;
typedef std::vector<RobotRegion> RobotRegionList;
typedef std::unordered_map<std::string /* Country */, uint32 /* LangType */> RobotLangContainer;
typedef std::vector<RobotDifficulty> RobotDifficultyList;
typedef std::unordered_map<uint8 /* CombatGrade */, RobotDifficultyList> RobotDifficultyListContainer;
typedef std::unordered_map<uint8 /* CombatGrade */, std::vector<double>> RobotDifficultyWeightListContainer;
typedef std::unordered_map<uint32 /* NatureType */, RobotNature> RobotNatureContainer;

typedef std::unordered_map<uint32/* SpawnId */, ItemSpawnInfo> ItemSpawnInfoMap;
typedef std::unordered_map<uint32/* TemplateId */, ItemTemplate> ItemTemplateContainer;
typedef std::unordered_map<uint32/* TemplateId */, ItemApplicationTemplate> ItemApplicationTemplateContainer;
typedef std::unordered_map<uint16 /* MapId */, ItemSpawnInfoMap> MapItemSpawnInfosContainer;

typedef std::vector<ItemBoxSpawnInfo> ItemBoxSpawnList;
typedef std::unordered_map<uint32/* TemplateId */, ItemBoxLootTemplate> ItemBoxLootTemplateMap;
typedef std::unordered_map<uint16 /* MapId */, ItemBoxSpawnList> MapItemBoxSpawnListContainer;
typedef std::unordered_map<uint32/* TemplateId */, ItemBoxTemplate> ItemBoxTemplateContainer;
typedef std::unordered_map<uint16/* MapId */, ItemBoxLootTemplateMap> MapItemBoxLootTemplatesContainer;

class ObjectMgr
{
public:
	static ObjectMgr* instance();
	bool load();

	CombatGrade const* getCombatGrade(uint8 grade) const;
	CombatGrade const* findCombatGrade(uint16 combatPower) const;
	float getStatWeight(StatIndex index) const { return m_statsWeights[index]; }
	float getStatsWeightSum() const { return m_statsWeightSum; }
	StatsRange const& getStatsRange() const { return m_statsRange; };
	uint16 calcUnitCombatPower(DataUnit const* data) const;
	UnitLevelXP const* getUnitLevelXP(uint8 level) const;
	RewardOnKillUnit const* getRewardOnKillUnit() const { return &m_rewardOnKillUnit; }
	UnitLootItemTemplate const* getUnitLootItemTemplate(uint32 itemClass, uint32 itemSubclass) const;
	ObjectDistributionList const* getUnitDistributionList(uint16 mapId) const;

	PlayerTemplate const* getPlayerTemplate(uint32 id) const;
	std::string generatePlayerName(LangType lang);
	RewardForRanking const* getRewardForRanking() const { return &m_rewardForRanking; }

	RobotTemplate const* getRobotTemplate(uint32 id) const;
	RobotSpawnList const* getRobotSpawnList(uint8 combatGrade) const;
	std::vector<double> const* getRobotSpawnWeightList(uint8 combatGrade) const;
	RobotNameList const* getRobotNameList(LangType lang) const;
	std::string const& getNextRobotNameByCountry(std::string const& country);
	std::string const& getRobotDefaultName(LangType lang) const;
	LangType getRobotLangByCountry(std::string const& country);
	RobotRegionList const* getRobotRegionList() const { return &m_robotRegionList; }
	std::vector<double> const* getRobotRegionWeightList() const { return &m_robotRegionWeightList; }
	RobotProficiency const* getRobotProficiency(uint8 proficiencyLevel) const;
	RobotDifficultyList const* getRobotDifficultyList(uint8 combatGrade) const;
	std::vector<double> const* getRobotDifficultyWeightList(uint8 combatGrade) const;
	RobotNature const* getRobotNature(uint32 natureType) const;

	ItemTemplate const* getItemTemplate(uint32 id) const;
	ItemApplicationTemplate const* getItemApplicationTemplate(uint32 id) const;
	ItemTemplate const* getMagicBeanTemplate() const;
	ItemTemplate const* getGoldTemplate() const;
	ItemSpawnInfoMap const* getItemSpawnInfos(uint16 mapId) const;
	ItemSpawnInfo const* getItemSpawnInfo(uint16 mapId, uint32 spawnId);

	ItemBoxTemplate const* getItemBoxTemplate(uint32 id) const;
	ItemBoxSpawnList const* getItemBoxSpawnList(uint16 mapId) const;
	ObjectDistributionList const* getItemBoxDistributionList(uint16 mapId) const;
	ItemBoxLootTemplate const* getItemBoxLootTemplate(uint16 mapId, uint32 lootId) const;

	uint32 generatePlayerSpawnId();
private:
	ObjectMgr();
	~ObjectMgr();

	bool loadUnitData();
	bool loadPlayerData();
	bool loadRobotData();
	bool loadItemBoxData();
	bool loadItemData();

	template <typename T>
	float calcStatCombatPower(T value, StatIndex index) const;
	uint16 calcUnitCombatPower(UnitStats const& stats) const;

	uint16 generatePlayerNameId();
	void generateItemBoxLootTemplates(uint16 mapId, std::unordered_map<uint32, std::vector<std::size_t>> const& itemBoxSpawnIndexGroups, std::unordered_map<uint32, std::vector<LootItem>>& itemGroups);

	std::string getDBFilePath(std::string const& dbFilename) const;

	uint32 m_playerSpawnIdCounter;
	uint16 m_playerNameIdCounter;

	CombatGradeList m_combatGradeList;
	StatsRange m_statsRange;
	float m_statsWeightSum;
	std::array<float, MAX_STAT_INDICES> m_statsWeights;
	std::array<UnitLevelXP, LEVEL_MAX> m_unitLevelXPList;
	RewardOnKillUnit m_rewardOnKillUnit;
	UnitLootItemTemplateContainer m_unitLootItemTemplateStore;
	MapObjectDistributionListContainer m_mapUnitDistributionListStore;

	PlayerTemplateContainer m_playerTemplateStore;
	PlayerNameContainer m_playerNameStore;
	RewardForRanking m_rewardForRanking;

	RobotTemplateContainer m_robotTemplateStore;
	std::array<UnitLevelXP, LEVEL_MAX> m_robotLevelXPList;
	GradedRobotSpawnListContainer m_gradedRobotSpawnListStore;
	GradedRobotSpawnWeightListContainer m_gradedRobotSpawnWeightListStore;
	RobotNameListContainer m_robotNameListStore;
	RobotRegionList m_robotRegionList;
	std::vector<double> m_robotRegionWeightList;
	RobotLangContainer m_robotLangStore;
	std::unordered_map<RobotNameList const*, uint32> m_robotNameIndexes;
	std::array<RobotProficiency, PROFICIENCY_LEVEL_MAX> m_robotProficiencyList;
	RobotDifficultyListContainer m_robotDifficultyListStore;
	RobotDifficultyWeightListContainer m_robotDifficultyWeightListStore;
	RobotNatureContainer m_robotNatureStore;

	ItemTemplateContainer m_itemTemplateStore;
	ItemApplicationTemplateContainer m_itemApplicationTemplateStore;
	uint32 m_magicBeanTemplateId;
	uint32 m_goldTemplateId;
	MapItemSpawnInfosContainer m_mapItemSpawnInfosStore;

	ItemBoxTemplateContainer m_itemBoxTemplateStore;
	MapItemBoxSpawnListContainer m_mapItemBoxSpawnListStore;
	MapItemBoxLootTemplatesContainer m_mapItemBoxLootTemplatesStore;
	MapObjectDistributionListContainer m_mapItemBoxDistributionListStore;
};

template<typename T>
inline float ObjectMgr::calcStatCombatPower(T value, StatIndex index) const
{
	float cp = value / (float)m_statsRange.maxValues[index] * (m_statsWeights[index] / m_statsWeightSum);
	return cp;
}


#define sObjectMgr ObjectMgr::instance()


#endif // __OBJECT_MGR_H__