#include "ObjectMgr.h"

#include "Application.h"
#include "logging/Log.h"
#include "ItemAssignHepler.h"

#define UNIT_DB_FILENAME							"unit.db"
#define ROBOT_DB_FILENAME							"robot.db"
#define PLAYER_DB_FILENAME							"player.db"
#define ITEM_DB_FILENAME							"item.db"
#define ITEMBOX_DB_FILENAME							"itembox.db"

#define MAX_PLAYER_NAME_ID_COUNTER					0x0FFF

// 默认的机器人名字语言
#define DEFAULT_ROBOT_NAME_LANG						LANG_enUS

ObjectMgr::ObjectMgr() :
	m_playerSpawnIdCounter(0),
	m_playerNameIdCounter(0),
	m_statsWeightSum(0),
	m_magicBeanTemplateId(0),
	m_goldTemplateId(0)
{
}

ObjectMgr::~ObjectMgr()
{

}

ObjectMgr* ObjectMgr::instance()
{
	static ObjectMgr instance;
	return &instance;
}

bool ObjectMgr::load()
{
	if (!this->loadUnitData())
		return false;

	if (!this->loadPlayerData())
		return false;

	if (!this->loadRobotData())
		return false;

	if (!this->loadItemData())
		return false;

	if (!this->loadItemBoxData())
		return false;

	return true;
}

CombatGrade const* ObjectMgr::getCombatGrade(uint8 grade) const
{
	if (grade >= 0 && grade < m_combatGradeList.size())
		return &m_combatGradeList[grade];

	return nullptr;
}

CombatGrade const* ObjectMgr::findCombatGrade(uint16 combatPower) const
{
	auto it = std::lower_bound(m_combatGradeList.begin(), m_combatGradeList.end(), combatPower, [](CombatGrade const& grade, uint16 cp) {
		return grade.maxCombatPower < cp;
	});

	if (it != m_combatGradeList.end())
		return &(*it);

	return nullptr;
}

uint16 ObjectMgr::calcUnitCombatPower(DataUnit const* data) const
{
	float cp = 0.f;
	cp += this->calcStatCombatPower(data->getMoveSpeed(), STATINDEX_MOVE_SPEED);
	cp += this->calcStatCombatPower(data->getAttackRange(), STATINDEX_ATTACK_RANGE);
	cp += this->calcStatCombatPower(data->getMaxHealth(), STATINDEX_MAX_HEALTH);
	cp += this->calcStatCombatPower(data->getHealthRegenRate(), STATINDEX_HEALTH_REGEN_RATE);
	cp += this->calcStatCombatPower(data->getMaxStamina(), STATINDEX_MAX_STAMINA);
	cp += this->calcStatCombatPower(data->getStaminaRegenRate(), STATINDEX_STAMINA_REGEN_RATE);
	cp += this->calcStatCombatPower(data->getAttackTakesStamina(), STATINDEX_ATTACK_TAKES_STAMINA);
	cp += this->calcStatCombatPower(data->getChargeConsumesStamina(), STATINDEX_CHARGE_CONSUMES_STAMINA);
	cp += this->calcStatCombatPower(data->getDamage(), STATINDEX_DAMAGE);
	uint16 ret = (uint16)std::round(cp * COMBAT_POWER_MAX);
	return ret;
}

UnitLevelXP const* ObjectMgr::getUnitLevelXP(uint8 level) const
{
	if (level > 0 && level <= LEVEL_MAX)
		return &m_unitLevelXPList[level - 1];

	return nullptr;
}

UnitLootItemTemplate const* ObjectMgr::getUnitLootItemTemplate(uint32 itemClass, uint32 itemSubclass) const
{
	auto it = m_unitLootItemTemplateStore.find(itemClass);
	if (it != m_unitLootItemTemplateStore.end())
	{
		auto const& templates = (*it).second;
		auto tmplIt = templates.find(itemSubclass);
		if (tmplIt != templates.end())
			return &((*tmplIt).second);
	}

	return nullptr;
}

ObjectDistributionList const* ObjectMgr::getUnitDistributionList(uint16 mapId) const
{
	auto it = m_mapUnitDistributionListStore.find(mapId);
	if (it == m_mapUnitDistributionListStore.end())
		return nullptr;

	return &(*it).second;
}

PlayerTemplate const* ObjectMgr::getPlayerTemplate(uint32 id) const
{
	auto it = m_playerTemplateStore.find(id);
	if (it != m_playerTemplateStore.end())
		return &((*it).second);

	return nullptr;
}

std::string ObjectMgr::generatePlayerName(LangType lang)
{
	auto it = m_playerNameStore.find(lang);
	if (it == m_playerNameStore.end())
		it = m_playerNameStore.find(DEFAULT_LANG);

	std::string name = StringUtil::format((*it).second.c_str(), this->generatePlayerNameId());
	return name;
}

RobotTemplate const* ObjectMgr::getRobotTemplate(uint32 id) const
{
	auto it = m_robotTemplateStore.find(id);
	if (it != m_robotTemplateStore.end())
		return &((*it).second);

	return nullptr;
}

ItemTemplate const* ObjectMgr::getItemTemplate(uint32 id) const
{
	auto it = m_itemTemplateStore.find(id);
	if (it != m_itemTemplateStore.end())
		return &((*it).second);

	return nullptr;
}

ItemApplicationTemplate const* ObjectMgr::getItemApplicationTemplate(uint32 id) const
{
	auto it = m_itemApplicationTemplateStore.find(id);
	if (it != m_itemApplicationTemplateStore.end())
		return &((*it).second);

	return nullptr;
}

ItemTemplate const* ObjectMgr::getMagicBeanTemplate() const
{
	if (m_magicBeanTemplateId > 0)
		return this->getItemTemplate(m_magicBeanTemplateId);
	return nullptr;
}

ItemTemplate const* ObjectMgr::getGoldTemplate() const
{
	if (m_goldTemplateId > 0)
		return this->getItemTemplate(m_goldTemplateId);
	return nullptr;
}

ItemSpawnInfoMap const* ObjectMgr::getItemSpawnInfos(uint16 mapId) const
{
	auto it = m_mapItemSpawnInfosStore.find(mapId);
	if (it == m_mapItemSpawnInfosStore.end())
		return nullptr;

	return &(*it).second;
}

ItemSpawnInfo const* ObjectMgr::getItemSpawnInfo(uint16 mapId, uint32 spawnId)
{
	auto spawnInfosIt = m_mapItemSpawnInfosStore.find(mapId);
	if (spawnInfosIt != m_mapItemSpawnInfosStore.end())
	{
		auto const& spawnInfos = (*spawnInfosIt).second;
		auto it = spawnInfos.find(spawnId);
		if (it != spawnInfos.end())
			return &(*it).second;
	}

	return nullptr;
}

bool ObjectMgr::loadUnitData()
{
	std::string dbFile = this->getDBFilePath(UNIT_DB_FILENAME);
	try
	{
		SQLite::Database db(dbFile);
		try
		{
			SQLite::Statement countQuery(db, "SELECT count(*) FROM combat_grade");
			if (countQuery.executeStep())
				m_combatGradeList.resize(countQuery.getColumn(0).getInt());

			SQLite::Statement query(db, "SELECT * FROM combat_grade ORDER BY grade ASC");
			while (query.executeStep())
			{
				CombatGrade combatGrade;
				combatGrade.grade = query.getColumn("grade");
				int32 index = combatGrade.grade;
				if (index > 0)
					combatGrade.minCombatPower = m_combatGradeList[index - 1].maxCombatPower + 1;
				else
					combatGrade.minCombatPower = 0;
				combatGrade.maxCombatPower = query.getColumn("combat_power_limit");
				combatGrade.loseMoneyPercent = (float)query.getColumn("lose_money_percent").getDouble();
				combatGrade.loseMoneyApportionment = (float)query.getColumn("lose_money_apportionment").getDouble();
				combatGrade.robotNoNameChance = (float)query.getColumn("robot_noname_chance").getDouble();
				combatGrade.dangerStateHealthLoss = query.getColumn("danger_state_health_loss");

				m_combatGradeList[index] = combatGrade;
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load combat grade failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement weightSumQuery(db, "SELECT SUM(weight) FROM stats_weight");
			if (weightSumQuery.executeStep())
				m_statsWeightSum = (float)weightSumQuery.getColumn(0).getDouble();
			NS_ASSERT(m_statsWeightSum > 0);
			SQLite::Statement query(db, "SELECT * FROM stats_weight");
			while (query.executeStep())
			{
				int32 statType = query.getColumn("stat_type");
				m_statsWeights[statType] = (float)query.getColumn("weight").getDouble();
				m_statsRange.minValues[statType] = (float)query.getColumn("min_value").getDouble();
				m_statsRange.maxValues[statType] = (float)query.getColumn("max_value").getDouble();
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load stats weight failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM unit_level_xp");
			while (query.executeStep())
			{
				uint8 level = query.getColumn("level");
				UnitLevelXP& levelXP = m_unitLevelXPList[level - 1];
				levelXP.experience = query.getColumn("experience");
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load unit level xp failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM reward_onkill_unit");
			if (query.executeStep())
			{
				m_rewardOnKillUnit.xp = query.getColumn("xp");
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load unit onkill reward failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM unit_lootitem_template");
			while (query.executeStep())
			{
				uint32 itemClass = query.getColumn("item_class");
				uint32 itemSubclass = query.getColumn("item_subclass");
				UnitLootItemTemplate& tmpl = m_unitLootItemTemplateStore[itemClass][itemSubclass];
				tmpl.itemClass = itemClass;
				tmpl.itemSubClass = itemSubclass;
				tmpl.chance = (float)query.getColumn("chance").getDouble();
				tmpl.maxCount = query.getColumn("max_count");
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load unit lootitem template failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM unit_distribution");
			while (query.executeStep())
			{
				uint16 mapId = query.getColumn("map_id");
				ObjectDistributionList& distrList = m_mapUnitDistributionListStore[mapId];
				ObjectDistribution distr;
				int32 spawnAreaX = query.getColumn("spawn_area_x");
				int32 spawnAreaY = query.getColumn("spawn_area_y");
				int32 spawnAreaWidth = query.getColumn("spawn_area_width");
				int32 spawnAreaHeight = query.getColumn("spawn_area_height");
				distr.spawnArea.lowBound.x = spawnAreaX;
				distr.spawnArea.lowBound.y = spawnAreaY;
				distr.spawnArea.highBound.x = spawnAreaX + spawnAreaWidth - 1;
				distr.spawnArea.highBound.y = spawnAreaY + spawnAreaHeight - 1;
				distrList.emplace_back(distr);
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load unit distribution failed. error: %s", e.what());
			return false;
		}
	}
	catch (std::exception& e)
	{
		NS_LOG_INFO("server.loading", "Failed to load DB file %s. error: %s", dbFile.c_str(), e.what());
		return false;
	}

	return true;
}

bool ObjectMgr::loadPlayerData()
{
	std::string dbFile = this->getDBFilePath(PLAYER_DB_FILENAME);
	try
	{
		SQLite::Database db(dbFile);
		try
		{
			SQLite::Statement query(db, "SELECT * FROM player_template");
			while (query.executeStep())
			{
				uint32 id = query.getColumn("id");
				PlayerTemplate& tmpl = m_playerTemplateStore[id];
				tmpl.id = id;
				tmpl.originLevel = query.getColumn("origin_level");
				tmpl.displayId = query.getColumn("display_id");
			}

			NS_LOG_INFO("server.loading", "Loaded %zu player templates.", m_playerTemplateStore.size());
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load player template failed. error: %s", e.what());
			return false;
		}

		try
		{
			for (auto it = m_playerTemplateStore.begin(); it != m_playerTemplateStore.end(); ++it)
			{
				PlayerTemplate& tmpl = (*it).second;

				SQLite::Statement countQuery(db, "SELECT count(*) FROM player_stage_stats WHERE template_id=:template_id");
				countQuery.bind(":template_id", (*it).first);
				if(countQuery.executeStep())
					tmpl.stageStatsList.resize(countQuery.getColumn(0).getInt());
				
				SQLite::Statement query(db, "SELECT * FROM player_stage_stats WHERE template_id=:template_id ORDER BY stage ASC");
				query.bind(":template_id", (*it).first);
				while (query.executeStep())
				{
					uint8 stage = query.getColumn("stage");
					UnitStats& stats = tmpl.stageStatsList[stage];
					stats.maxHealth = query.getColumn("max_health");
					stats.healthRegenRate = (float)query.getColumn("health_regen_rate").getDouble();
					stats.attackRange = (float)query.getColumn("attack_range").getDouble();
					stats.moveSpeed = query.getColumn("move_speed");
					stats.maxStamina = query.getColumn("max_stamina");
					stats.staminaRegenRate = (float)query.getColumn("stamina_regen_rate").getDouble();
					stats.attackTakesStamina = query.getColumn("attack_takes_stamina");
					stats.chargeConsumesStamina = query.getColumn("charge_consumes_stamina");
					stats.damage = query.getColumn("damage");
				}
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load player stage stats failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM player_name");
			while (query.executeStep())
			{
				std::string name = query.getColumn("name");
				uint32 lang = query.getColumn("lang");
				m_playerNameStore[lang] = name;
			}
			NS_ASSERT(m_playerNameStore.find(DEFAULT_LANG) != m_playerNameStore.end(), "No player name specified for the default lang '%s'.", langTags[DEFAULT_LANG]);
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load player name failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM reward_for_ranking");
			if (query.executeStep())
			{
				m_rewardForRanking.baseXP = query.getColumn("xp_base");
				m_rewardForRanking.maxXP = query.getColumn("xp_max");
				m_rewardForRanking.moneyRewardedPlayersPercent = query.getColumn("money_rewarded_players_percent");
				m_rewardForRanking.baseMoney = query.getColumn("money_base");
				m_rewardForRanking.maxMoney = query.getColumn("money_max");
				m_rewardForRanking.xpRewardedPlayersPercent = query.getColumn("xp_rewarded_players_percent");
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load reward for ranking failed. error: %s", e.what());
			return false;
		}
	}
	catch (std::exception& e)
	{
		NS_LOG_INFO("server.loading", "Failed to load DB file %s. error: %s", dbFile.c_str(),  e.what());
		return false;
	}

	return true;
}

bool ObjectMgr::loadRobotData()
{
	std::string dbFile = this->getDBFilePath(ROBOT_DB_FILENAME);
	try
	{
		SQLite::Database db(dbFile);

		try
		{
			SQLite::Statement query(db, "SELECT * FROM robot_template");

			while (query.executeStep())
			{
				uint32 id = query.getColumn("id");
				RobotTemplate& tmpl = m_robotTemplateStore[id];
				tmpl.id = id;
				tmpl.displayId = query.getColumn("display_id");
			}
			NS_LOG_INFO("server.loading", "Loaded %zu robot templates.", m_robotTemplateStore.size());
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load robot template failed. error: %s", e.what());
			return false;
		}

		try
		{
			for (auto tmplIt = m_robotTemplateStore.begin(); tmplIt != m_robotTemplateStore.end(); ++tmplIt)
			{
				RobotTemplate& tmpl = (*tmplIt).second;

				SQLite::Statement countQuery(db, "SELECT count(*) FROM robot_stage_stats WHERE template_id=:template_id");
				countQuery.bind(":template_id", (*tmplIt).first);
				if (countQuery.executeStep())
					tmpl.stageStatsList.resize(countQuery.getColumn(0).getInt());

				SQLite::Statement query(db, "SELECT * FROM robot_stage_stats WHERE template_id=:template_id ORDER BY stage ASC");
				query.bind(":template_id", (*tmplIt).first);
				while (query.executeStep())
				{
					uint8 stage = query.getColumn("stage");
					RobotStats& stats = tmpl.stageStatsList[stage];
					stats.maxHealth = query.getColumn("max_health");
					stats.healthRegenRate = (float)query.getColumn("health_regen_rate").getDouble();
					stats.attackRange = (float)query.getColumn("attack_range").getDouble();
					stats.moveSpeed = query.getColumn("move_speed");
					stats.maxStamina = query.getColumn("max_stamina");
					stats.staminaRegenRate = (float)query.getColumn("stamina_regen_rate").getDouble();
					stats.attackTakesStamina = query.getColumn("attack_takes_stamina");
					stats.chargeConsumesStamina = query.getColumn("charge_consumes_stamina");
					stats.damage = query.getColumn("damage");
					stats.combatPower = query.getColumn("combat_power");

					tmpl.combatPowerStages.emplace(stats.combatPower, stage);
				}
			}

		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load robot stage stats failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM robot_spawn_info");
			std::unordered_map<uint8, double> weightSums;
			while (query.executeStep())
			{
				uint8 combatGrade = query.getColumn("combat_grade");
				RobotSpawnList& robotSpawnList = m_gradedRobotSpawnListStore[combatGrade];
				RobotSpawnInfo spawnInfo;
				spawnInfo.combatGrade = combatGrade;
				spawnInfo.templateId = query.getColumn("template_id");
				spawnInfo.minLevel = query.getColumn("level_min");
				spawnInfo.maxLevel = query.getColumn("level_max");
				robotSpawnList.emplace_back(spawnInfo);

				double weight = query.getColumn("weight").getDouble();
				m_gradedRobotSpawnWeightListStore[combatGrade].emplace_back(weight);
				weightSums[combatGrade] += weight;
			}

			for (auto it = weightSums.begin(); it != weightSums.end(); ++it)
			{
				double weightSum = (*it).second;
				if (weightSum <= 0.0)
				{
					auto& weights = m_gradedRobotSpawnWeightListStore[(*it).first];
					weights.assign(weights.size(), 1.0);
				}
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load robot spawn info failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM robot_name");
			while (query.executeStep())
			{
				uint32 lang = query.getColumn("lang");
				auto& nameList = m_robotNameListStore[lang];
				std::string name =  query.getColumn("name");
				nameList.emplace_back(name);
			}
			NS_ASSERT(m_robotNameListStore.find(DEFAULT_ROBOT_NAME_LANG) != m_robotNameListStore.end(), "No robot name specified for the default lang '%s'.", langTags[DEFAULT_ROBOT_NAME_LANG]);
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load robot name failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM robot_region");
			double weightSum = 0.0;
			while (query.executeStep())
			{
				std::string country = query.getColumn("country");
				uint32 lang = query.getColumn("lang");
				RobotRegion region;
				region.country = country;
				region.lang = (LangType)lang;
				double weight = query.getColumn("weight").getDouble();

				m_robotRegionList.emplace_back(region);
				m_robotLangStore[country] = lang;
				m_robotRegionWeightList.emplace_back(weight);
				weightSum += weight;
			}

			if (weightSum <= 0.0)
				m_robotRegionWeightList.assign(m_robotRegionWeightList.size(), 1.0);
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load robot country failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM robot_proficiency");
			while (query.executeStep())
			{
				uint8 level = query.getColumn("level");
				RobotProficiency& proficiency = m_robotProficiencyList[level - 1];
				proficiency.minDodgeReactionTime = query.getColumn("dodge_reaction_time_min");
				proficiency.maxDodgeReactionTime = query.getColumn("dodge_reaction_time_max");
				proficiency.effectiveDodgeChance = (float)query.getColumn("effective_dodge_chance").getDouble();
				proficiency.minTargetReactionTime = query.getColumn("target_reaction_time_min");
				proficiency.maxTargetReactionTime = query.getColumn("target_reaction_time_max");
				proficiency.aimingAccuracy = (float)query.getColumn("aiming_accuracy").getDouble();
				proficiency.minContinuousAttackDelay = query.getColumn("continuous_attack_delay_min");
				proficiency.maxContinuousAttackDelay = query.getColumn("continuous_attack_delay_max");
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load robot proficiency failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM robot_difficulty ORDER BY combat_grade ASC, proficiency_level ASC");
			std::unordered_map<uint8, double> weightSums;
			while (query.executeStep())
			{
				uint8 combatGrade = query.getColumn("combat_grade");
				RobotDifficultyList& difficultyList = m_robotDifficultyListStore[combatGrade];
				RobotDifficulty difficulty;
				difficulty.combatGrade = combatGrade;
				difficulty.proficiencyLevel = query.getColumn("proficiency_level");
				difficulty.maxCombatPower = query.getColumn("max_combat_power");
				difficulty.natureType = query.getColumn("nature_type");
				difficultyList.emplace_back(difficulty);

				double weight = query.getColumn("weight").getDouble();
				m_robotDifficultyWeightListStore[combatGrade].emplace_back(weight);
				weightSums[combatGrade] += weight;
			}

			for (auto it = weightSums.begin(); it != weightSums.end(); ++it)
			{
				double weightSum = (*it).second;
				if (weightSum <= 0.0)
				{
					auto& weights = m_robotDifficultyWeightListStore[(*it).first];
					weights.assign(weights.size(), 1.0);
				}
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load robot difficulty failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM robot_nature");
			while (query.executeStep())
			{
				uint32 type = query.getColumn("type");
				uint32 effectType = query.getColumn("effect_type");
				int32 effectValue = query.getColumn("effect_value");
				RobotNature& nature = m_robotNatureStore[type];
				nature.effects[effectType] = effectValue;
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load robot nature failed. error: %s", e.what());
			return false;
		}
	}
	catch (std::exception& e)
	{
		NS_LOG_INFO("server.loading", "Failed to load DB file %s. error: %s", dbFile.c_str(), e.what());
		return false;
	}

	return true;
}

bool ObjectMgr::loadItemBoxData()
{
	std::string dbFile = this->getDBFilePath(ITEMBOX_DB_FILENAME);
	try
	{
		SQLite::Database db(dbFile);
		try
		{
			SQLite::Statement query(db, "SELECT * FROM itembox_template");

			while (query.executeStep())
			{
				uint32 id = query.getColumn("id");
				ItemBoxTemplate& tmpl = m_itemBoxTemplateStore[id];
				tmpl.id = id;
				tmpl.maxHealth = query.getColumn("max_health");
			}

			NS_LOG_INFO("server.loading", "Loaded %zu itembox templates.", m_itemBoxTemplateStore.size());
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load itembox template failed. error: %s", e.what());
			return false;
		}

		std::unordered_map<uint16 /* MapId */, std::unordered_map<uint32 /* LootGroupId */, std::vector<std::size_t /* ItemBoxSpawnIndex */>>> groupedItemBoxSpawnIndexes;
		try
		{
			SQLite::Statement query(db, "SELECT * FROM itembox_spawn_info");
			while (query.executeStep())
			{
				uint16 mapId = query.getColumn("map_id");
				uint32 lootGroupId = query.getColumn("loot_group_id");
				ItemBoxSpawnList& spawnList = m_mapItemBoxSpawnListStore[mapId];
				ItemBoxSpawnInfo spawnInfo;
				spawnInfo.templateId = query.getColumn("template_id");
				spawnInfo.lootId = 0;
				spawnList.emplace_back(spawnInfo);
				auto spawnIndex = spawnList.size() - 1;
				groupedItemBoxSpawnIndexes[mapId][lootGroupId].emplace_back(spawnIndex);
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load itembox spawn info failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM itembox_distribution");
			while (query.executeStep())
			{
				uint16 mapId = query.getColumn("map_id");
				ObjectDistributionList& distrList = m_mapItemBoxDistributionListStore[mapId];
				ObjectDistribution distr;
				int32 spawnAreaX = query.getColumn("spawn_area_x");
				int32 spawnAreaY = query.getColumn("spawn_area_y");
				int32 spawnAreaWidth = query.getColumn("spawn_area_width");
				int32 spawnAreaHeight = query.getColumn("spawn_area_height");
				distr.spawnArea.lowBound.x = spawnAreaX;
				distr.spawnArea.lowBound.y = spawnAreaY;
				distr.spawnArea.highBound.x = spawnAreaX + spawnAreaWidth - 1;
				distr.spawnArea.highBound.y = spawnAreaY + spawnAreaHeight - 1;
				distrList.emplace_back(distr);
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load itembox distribution failed. error: %s", e.what());
			return false;
		}

		std::unordered_map<uint32/* LootGroupId */, std::vector<LootItem>> itemGroups;
		try
		{
			SQLite::Statement query(db, "SELECT * FROM itembox_loot_template");
			while (query.executeStep())
			{
				uint32 groupId = query.getColumn("group_id");
				auto& itemList = itemGroups[groupId];
				LootItem lootItem;
				lootItem.itemId = (uint16)query.getColumn("item_id");
				lootItem.chance = (float)query.getColumn("chance").getDouble();
				lootItem.minCount = query.getColumn("min_count");
				lootItem.maxCount = query.getColumn("max_count");
				itemList.emplace_back(lootItem);
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load itembox loot template failed. error: %s", e.what());
			return false;
		}

		for (auto pr : groupedItemBoxSpawnIndexes)
		{
			uint16 mapId = pr.first;
			auto const& spawnIndexGroups = pr.second;
			this->generateItemBoxLootTemplates(mapId, spawnIndexGroups, itemGroups);
		}
	}
	catch (std::exception& e)
	{
		NS_LOG_INFO("server.loading", "Failed to load DB file %s. error: %s", dbFile.c_str(), e.what());
		return false;
	}

	return true;
}

bool ObjectMgr::loadItemData()
{
	std::string dbFile = this->getDBFilePath(ITEM_DB_FILENAME);
	try
	{
		SQLite::Database db(dbFile);
		try
		{
			SQLite::Statement query(db, "SELECT * FROM item_template");

			while (query.executeStep())
			{
				uint32 id = query.getColumn("id");
				ItemTemplate& tmpl = m_itemTemplateStore[id];
				tmpl.id = id;
				tmpl.displayId = query.getColumn("display_id");
				tmpl.itemClass = query.getColumn("class");
				tmpl.itemSubClass = query.getColumn("subclass");
				tmpl.level = (uint8)query.getColumn("level").getUInt();
				tmpl.stackable = query.getColumn("stackable");
				tmpl.appId = query.getColumn("app_id");
				int32 index = query.getColumnIndex("stat_type1");
				for (int32 i = 0; i < MAX_ITEM_STATS; ++i)
				{
					ItemStat stat;
					stat.type = query.getColumn(StringUtil::format("stat_type%d", (i + 1)).c_str());
					if (stat.type != ITEM_STAT_NONE)
					{
						stat.value = query.getColumn(StringUtil::format("stat_value%d", (i + 1)).c_str());
						tmpl.itemStats.emplace_back(stat);
					}
				}

				switch (tmpl.itemClass)
				{
				case ITEM_CLASS_MAGIC_BEAN:
					if (tmpl.itemSubClass == ITEM_SUBCLASS_MAGIC_BEAN_SPARRING)
					{
						NS_ASSERT(m_magicBeanTemplateId == 0, "Only one MagicBean sparring template can be defined.");
						m_magicBeanTemplateId = id;
					}
					break;
				case ITEM_CLASS_GOLD:
					NS_ASSERT(m_goldTemplateId == 0, "Only one Gold template can be defined.");
					m_goldTemplateId = id;
					break;
				default:
					break;
				}
			}

			NS_LOG_INFO("server.loading", "Loaded %zu item templates.", m_itemTemplateStore.size());
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load item template failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM item_application_template");

			while (query.executeStep())
			{
				uint32 id = query.getColumn("id");
				ItemApplicationTemplate& tmpl = m_itemApplicationTemplateStore[id];
				tmpl.id = id;
				tmpl.flags = query.getColumn("flags");
				tmpl.visualId = query.getColumn("visual_id");
				tmpl.duration = query.getColumn("duration");
				tmpl.recoveryTime = query.getColumn("recovery_time");
				int32 index = query.getColumnIndex("effect_type1");
				for (int32 i = 0; i < MAX_ITEM_EFFECTS; ++i)
				{
					ItemEffect effect;
					effect.type = query.getColumn(StringUtil::format("effect_type%d", (i + 1)).c_str());
					if (effect.type != ITEM_EFFECT_NONE)
					{
						effect.value = query.getColumn(StringUtil::format("effect_value%d", (i + 1)).c_str());
						tmpl.effects.emplace_back(effect);
					}
				}
			}

			NS_LOG_INFO("server.loading", "Loaded %zu item application templates.", m_itemApplicationTemplateStore.size());
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load item application template failed. error: %s", e.what());
			return false;
		}

		try
		{
			SQLite::Statement query(db, "SELECT * FROM item_spawn_info");
			while (query.executeStep())
			{
				uint16 mapId = query.getColumn("map_id");
				uint32 id = query.getColumn("id");
				ItemSpawnInfo& spawnInfo = m_mapItemSpawnInfosStore[mapId][id];
				spawnInfo.id = id;
				spawnInfo.itemId = query.getColumn("item_id");
				spawnInfo.minCount = query.getColumn("min_count");
				spawnInfo.maxCount = query.getColumn("max_count");
				spawnInfo.spawnPos.x = query.getColumn("spawn_pos_x");
				spawnInfo.spawnPos.y = query.getColumn("spawn_pos_y");
				spawnInfo.spawnDist = query.getColumn("spawn_dist");
				spawnInfo.spawnTime = query.getColumn("spawn_time");
			}
		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load item spawn info failed. error: %s", e.what());
			return false;
		}
	}
	catch (std::exception& e)
	{
		NS_LOG_INFO("server.loading", "Failed to load DB file %s. error: %s", dbFile.c_str(), e.what());
		return false;
	}

	return true;
}

RobotSpawnList const* ObjectMgr::getRobotSpawnList(uint8 combatGrade) const
{
	auto it = m_gradedRobotSpawnListStore.find(combatGrade);
	if (it == m_gradedRobotSpawnListStore.end())
		return nullptr;

	return &(*it).second;
}

std::vector<double> const* ObjectMgr::getRobotSpawnWeightList(uint8 combatGrade) const
{
	auto it = m_gradedRobotSpawnWeightListStore.find(combatGrade);
	if (it == m_gradedRobotSpawnWeightListStore.end())
		return nullptr;

	return &(*it).second;
}

RobotNameList const* ObjectMgr::getRobotNameList(LangType lang) const
{
	auto it = m_robotNameListStore.find(lang);
	if(it != m_robotNameListStore.end())
		return &(*it).second;

	it = m_robotNameListStore.find(DEFAULT_ROBOT_NAME_LANG);
	if (it != m_robotNameListStore.end())
		return &(*it).second;

	return nullptr;
}

std::string const& ObjectMgr::getNextRobotNameByCountry(std::string const& country)
{
	LangType lang = this->getRobotLangByCountry(country);
	auto* nameList = sObjectMgr->getRobotNameList(lang);
	NS_ASSERT(nameList);
	uint32& nameIndex = m_robotNameIndexes[nameList];
	++nameIndex;
	std::string const& name = (*nameList)[nameIndex];
	if (nameIndex >= nameList->size() - 1)
		nameIndex = 0;

	return name;
}

std::string const& ObjectMgr::getRobotDefaultName(LangType lang) const
{
	auto* nameList = sObjectMgr->getRobotNameList(lang);
	NS_ASSERT(nameList);
	std::string const& name = (*nameList)[0];
	return name;
}

LangType ObjectMgr::getRobotLangByCountry(std::string const& country)
{
	LangType lang = DEFAULT_ROBOT_NAME_LANG;
	auto it = m_robotLangStore.find(country);
	if (it != m_robotLangStore.end())
		lang = (LangType)((*it).second);

	return lang;
}

RobotProficiency const* ObjectMgr::getRobotProficiency(uint8 proficiencyLevel) const
{
	if (proficiencyLevel > 0 && proficiencyLevel <= PROFICIENCY_LEVEL_MAX)
		return &m_robotProficiencyList[proficiencyLevel - 1];

	return nullptr;
}

RobotDifficultyList const* ObjectMgr::getRobotDifficultyList(uint8 combatGrade) const
{
	auto it = m_robotDifficultyListStore.find(combatGrade);
	if (it == m_robotDifficultyListStore.end())
		return nullptr;

	return &(*it).second;
}

std::vector<double> const* ObjectMgr::getRobotDifficultyWeightList(uint8 combatGrade) const
{
	auto it = m_robotDifficultyWeightListStore.find(combatGrade);
	if (it == m_robotDifficultyWeightListStore.end())
		return nullptr;

	return &(*it).second;
}

RobotNature const* ObjectMgr::getRobotNature(uint32 natureType) const
{
	auto it = m_robotNatureStore.find(natureType);
	if (it == m_robotNatureStore.end())
		return nullptr;

	return &(*it).second;
}

uint16 ObjectMgr::calcUnitCombatPower(UnitStats const& stats) const
{
	float cp = 0.f;
	cp += this->calcStatCombatPower(stats.moveSpeed, STATINDEX_MOVE_SPEED);
	cp += this->calcStatCombatPower(stats.attackRange, STATINDEX_ATTACK_RANGE);
	cp += this->calcStatCombatPower(stats.maxHealth, STATINDEX_MAX_HEALTH);
	cp += this->calcStatCombatPower(stats.healthRegenRate, STATINDEX_HEALTH_REGEN_RATE);
	cp += this->calcStatCombatPower(stats.maxStamina, STATINDEX_MAX_STAMINA);
	cp += this->calcStatCombatPower(stats.staminaRegenRate, STATINDEX_STAMINA_REGEN_RATE);
	cp += this->calcStatCombatPower(stats.attackTakesStamina, STATINDEX_ATTACK_TAKES_STAMINA);
	cp += this->calcStatCombatPower(stats.chargeConsumesStamina, STATINDEX_CHARGE_CONSUMES_STAMINA);
	cp += this->calcStatCombatPower(stats.damage, STATINDEX_DAMAGE);
	uint16 ret = (uint16)std::round(cp * COMBAT_POWER_MAX);
	return ret;
}

uint16 ObjectMgr::generatePlayerNameId()
{
	if (m_playerNameIdCounter >= MAX_PLAYER_NAME_ID_COUNTER)
		m_playerNameIdCounter = 0;
	++m_playerNameIdCounter;

	uint16 magic = (uint16)random(1, 15);
	uint16 id = magic << 12 | m_playerNameIdCounter;

	return id;
}

void ObjectMgr::generateItemBoxLootTemplates(uint16 mapId, std::unordered_map<uint32, std::vector<std::size_t>> const& itemBoxSpawnIndexGroups, std::unordered_map<uint32, std::vector<LootItem>>& itemGroups)
{
	uint32 lootIdCounter = 0;
	for (auto group : itemBoxSpawnIndexGroups)
	{
		uint32 groupId = group.first;
		auto const& spawnIndexList = group.second;

		auto it = itemGroups.find(groupId);
		NS_ASSERT(it != itemGroups.end());
		auto& itemList = (*it).second;

		int32 numSpawn = (int32)spawnIndexList.size();
		std::vector<SortedItemBox> itemBoxList;
		itemBoxList.resize(numSpawn);
		int32 itemCount = assignItemsToItemBoxs(itemList, itemBoxList);

		for (auto i = 0; i < itemBoxList.size(); ++i)
		{
			auto const& itemBox = itemBoxList[i];
			uint32 lootId = ++lootIdCounter;
			ItemBoxLootTemplate& tmpl = m_mapItemBoxLootTemplatesStore[mapId][lootId];
			tmpl.id = lootId;
			auto spawnIndex = spawnIndexList[i];
			auto& spawnInfo = m_mapItemBoxSpawnListStore[mapId][spawnIndex];
			spawnInfo.lootId = lootId;
			NS_LOG_DEBUG("server.loading", "==== ItemBoxLootTemplate LootId: %d ItemCount: %d ====", lootId, (int32)itemBox.getItemCount());

			auto const& itemList = itemBox.getItemList();
			for (auto const& item : itemList)
			{
				tmpl.itemList.emplace_back(item);
				NS_LOG_DEBUG("server.loading", "ItemId: %d Chance: %.1f Count: %d-%d", item.itemId, item.chance, item.minCount, item.maxCount);

				--itemCount;
				NS_ASSERT(itemCount >= 0);
			}
		}
	}
}

ItemBoxTemplate const* ObjectMgr::getItemBoxTemplate(uint32 id) const
{
	auto it = m_itemBoxTemplateStore.find(id);
	if (it != m_itemBoxTemplateStore.end())
		return &((*it).second);

	return nullptr;
}

ItemBoxSpawnList const* ObjectMgr::getItemBoxSpawnList(uint16 mapId) const
{
	auto it = m_mapItemBoxSpawnListStore.find(mapId);
	if (it == m_mapItemBoxSpawnListStore.end())
		return nullptr;

	return &(*it).second;
}

ObjectDistributionList const* ObjectMgr::getItemBoxDistributionList(uint16 mapId) const
{
	auto it = m_mapItemBoxDistributionListStore.find(mapId);
	if (it == m_mapItemBoxDistributionListStore.end())
		return nullptr;

	return &(*it).second;
}

ItemBoxLootTemplate const* ObjectMgr::getItemBoxLootTemplate(uint16 mapId, uint32 lootId) const
{
	auto templatesIt = m_mapItemBoxLootTemplatesStore.find(mapId);
	if (templatesIt != m_mapItemBoxLootTemplatesStore.end())
	{
		auto const& templates = (*templatesIt).second;
		auto it = templates.find(lootId);
		if (it != templates.end())
			return &((*it).second);
	}

	return nullptr;
}

uint32 ObjectMgr::generatePlayerSpawnId()
{
	++m_playerSpawnIdCounter;
	NS_ASSERT(m_playerSpawnIdCounter <= uint32(0xFFFFFF), "Player spawn id overflow!");
	return m_playerSpawnIdCounter;
}

std::string ObjectMgr::getDBFilePath(std::string const& dbFilename) const
{
	return StringUtil::format("%sodb/%s", sApplication->getRoot(), dbFilename.c_str());
}



