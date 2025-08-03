#ifndef __DATA_UNIT_H__
#define __DATA_UNIT_H__

#include "utilities/Random.h"
#include "game/utils/MathTools.h"
#include "DataWorldObject.h"
#include "MovementInfo.h"
#include "StaminaInfo.h"
#include "UnitSlotDefines.h"

enum MovementFlag
{
	MOVEMENT_FLAG_NONE				= 0,
	MOVEMENT_FLAG_WALKING			= 1 << 0,
	MOVEMENT_FLAG_HANDUP			= 1 << 1,
};

enum StaminaFlag
{
	STAMINA_FLAG_NONE				= 0,
	STAMINA_FLAG_ATTACK				= 1 << 0,
	STAMINA_FLAG_CHARGING			= 1 << 1,
};

// 隐蔽状态
enum ConcealmentState
{
	CONCEALMENT_STATE_EXPOSED,
	CONCEALMENT_STATE_CONCEALING,
	CONCEALMENT_STATE_CONCEALED,
};

// 英雄角色ID
// 与玩家和机器人模板ID相同
enum HeroID
{
	HERO_NONE			= 0,
	HERO_BOY			= 1,
	HERO_LILY			= 2,
	HERO_PENGUIN		= 3,
	HERO_RABBIT			= 4,
	HERO_TEENGIRL		= 5,
	HERO_ELK			= 6,
	HERO_SANTA			= 7,
	HERO_BEAR			= 8,
};

enum StatType
{
	STAT_MAX_HEALTH,
	STAT_HEALTH_REGEN_RATE,
	STAT_ATTACK_RANGE,
	STAT_MOVE_SPEED,
	STAT_MAX_STAMINA,
	STAT_STAMINA_REGEN_RATE,
	STAT_ATTACK_TAKES_STAMINA,
	STAT_DAMAGE,
	STAT_CHARGE_CONSUMES_STAMINA,
	STAT_DEFENSE,
	MAX_STAT_TYPES,
};

#define	MAX_ATTACK_RANGE				343.0f	// 最大攻击范围。单位：points
#define MAX_MOVE_SPEED					150		// 最大移动速度。单位：points/second

// 单位的等级范围
#define LEVEL_MIN			1
#define LEVEL_MAX			200

// 战斗力最大值
#define COMBAT_POWER_MAX	1000

// 默认的国家
#define COUNTRY_DEFAULT		"US"

// 表情代码
enum SmileyCode
{
	SMILEY_NONE				= 0,	// 无
	SMILEY_LAUGH			= 1,	// 笑
	SMILEY_NAUGHTY			= 2,	// 淘气
	SMILEY_DEVIL			= 3,	// 魔鬼
	SMILEY_SAD				= 4,	// 伤心
	SMILEY_CRY				= 5,	// 哭
	SMILEY_ANGRY			= 6,	// 愤怒
};

enum UnitFlag: uint32
{
	UNIT_FLAG_NONE				= 0,
	UNIT_FLAG_DEATH_LOSE_MONEY	= 1 << 0,	// 在单位死亡（生命值为0）时是否失去钱币
	UNIT_FLAG_DAMAGED			= 1 << 1,	// 单位受到伤害
};

struct CombatGrade
{
	uint8 grade;
	uint16 minCombatPower;
	uint16 maxCombatPower;
	float loseMoneyPercent;
	float loseMoneyApportionment;
	float robotNoNameChance;
	int32 dangerStateHealthLoss;	// 当单位位于危险区时每秒失去的生命值
};

#define TRAINING_GROUND_COMBAT_GRADE					0	// 训练场的战斗等级

struct UnitStats
{
	int32 moveSpeed;
	float attackRange;
	int32 maxHealth;
	float healthRegenRate;
	int32 maxStamina;
	float staminaRegenRate;
	int32 attackTakesStamina;
	int32 chargeConsumesStamina;
	int32 damage;
};

// 衡量战斗力的属性指标
enum StatIndex
{
	STATINDEX_MAX_HEALTH,
	STATINDEX_HEALTH_REGEN_RATE,
	STATINDEX_ATTACK_RANGE,
	STATINDEX_MOVE_SPEED,
	STATINDEX_MAX_STAMINA,
	STATINDEX_STAMINA_REGEN_RATE,
	STATINDEX_ATTACK_TAKES_STAMINA,
	STATINDEX_CHARGE_CONSUMES_STAMINA,
	STATINDEX_DAMAGE,
	MAX_STAT_INDICES,
};

struct StatsRange
{
	std::array<float, MAX_STAT_INDICES> minValues;
	std::array<float, MAX_STAT_INDICES> maxValues;
};

struct RewardOnKillUnit
{
	int32 xp;
};

struct UnitLevelXP
{
	int32 experience;
};

struct UnitLootItemTemplate
{
	uint32 itemClass;
	uint32 itemSubClass;
	float chance;
	int32 maxCount;
};

class DataUnit : public DataWorldObject
{
public:
	enum Direction
	{
		UP,
		RIGHT_UP,
		RIGHT,
		RIGHT_DOWN,
		DOWN,
		LEFT_DOWN,
		LEFT,
		LEFT_UP,
	};

	DataUnit();
	virtual ~DataUnit();

	virtual void writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const override;

	virtual void setGuid(ObjectGuid const& guid) override;

	Point const& getLaunchCenter() const { return m_launchCenter; }
	void setLaunchCenter(const Point& center) { m_launchCenter = center; }
	float getLaunchRadiusInMap() const { return m_launchRadiusInMap; };
	void setLaunchRadiusInMap(float radius) { m_launchRadiusInMap = radius; }

	virtual Point const& getPosition() const override { return m_movementInfo.position; }
	virtual void setPosition(Point const& position) override { m_movementInfo.position = position; }

	// 名称
	void setName(std::string const& name) { m_name = name; }
	std::string const& getName() const { return m_name; }

	// 显示ID
	uint32 getDisplayId() const { return m_displayId; }
	void setDisplayId(uint32 id) { m_displayId = id; }

	 // 国家
	void setCountry(std::string const& country) { m_country = country; }
	std::string const& getCountry() const { return m_country; }

	// 出生点
	void setSpawnPoint(TileCoord const& spawnPoint) { m_spawnPoint = spawnPoint; }
	TileCoord const& getSpawnPoint() const { return m_spawnPoint; }

	// 等级
	void setLevel(uint8 level);
	uint8 getLevel() const { return m_level; }
	// 经验值
	virtual void setExperience(int32 xp) { m_experience = xp; }
	int32 getExperience() const { return m_experience; }
	// 下一个等级所需的经验值
	virtual void setNextLevelXP(int32 xp) { m_nextLevelXP = xp; }
	int32 getNextLevelXP() const { return m_nextLevelXP; }

	float getOrientation() const { return m_movementInfo.orientation; }
	void setOrientation(float rad);
	void randomOrientation() { this->setOrientation(float(random((float)-M_PI, (float)M_PI)));}

	// 攻击范围。单位：points
	void setAttackRange(float range);
	float getAttackRange() const { return m_attackRange; }
	void setBaseAttackRange(float range) { m_baseAttackRange = range; }
	float getBaseAttackRange() const { return m_baseAttackRange; }

	// 移动速度。单位：points/second
	int32 getMoveSpeed() const { return m_moveSpeed; }
	void setMoveSpeed(int32 moveSpeed);
	int32 getBaseMoveSpeed() const { return m_baseMoveSpeed; }
	void setBaseMoveSpeed(int32 moveSpeed) { m_baseMoveSpeed = moveSpeed; }

	// 生命值
	int32 getHealth() const { return m_health; }
	void setHealth(int32 health);
	// 最大生命值
	int32 getMaxHealth() const { return m_maxHealth; }
	void setMaxHealth(int32 maxHealth);
	int32 getBaseMaxHealth() const { return m_baseMaxHealth; }
	void setBaseMaxHealth(int32 maxHealth) { m_baseMaxHealth = maxHealth; }
	// 生命值恢复比率。单位：生命值恢复比率/minute
	void setHealthRegenRate(float rate) { m_healthRegenRate = rate; }
	float getHealthRegenRate() const { return m_healthRegenRate; }
	void setBaseHealthRegenRate(float rate) { m_baseHealthRegenRate = rate; }
	float getBaseHealthRegenRate() const { return m_baseHealthRegenRate; }

	// 体力值
	void setStamina(int32 stamina) { m_staminaInfo.stamina = stamina; }
	int32 getStamina() const { return m_staminaInfo.stamina; }
	StaminaInfo const& getStaminaInfo() const { return m_staminaInfo; }
	// 体力计数器
	void increaseStaminaCounter() { ++m_staminaInfo.counter; }
	uint32 getStaminaCounter() const { return m_staminaInfo.counter; }
	// 攻击计数器
	uint32 getAttackCounter() const { return m_staminaInfo.attackCounter; }
	void increaseAttackCounter() { ++m_staminaInfo.attackCounter; }
	void resetAttackCounter() { m_staminaInfo.attackCounter = 0; }
	// 消耗体力总和
	uint32 getConsumedStaminaTotal() const { return m_staminaInfo.consumedStaminaTotal; }
	void increaseConsumedStaminaTotal(uint32 amount) { m_staminaInfo.consumedStaminaTotal += amount; }
	void resetConsumedStaminaTotal() { m_staminaInfo.consumedStaminaTotal = 0; }
	// 体力标记
	void setStaminaFlags(uint32 flags) { m_staminaInfo.flags = flags; }
	void addStaminaFlag(uint32 flag) { if (!this->hasStaminaFlag(flag)) m_staminaInfo.flags |= flag; }
	bool hasStaminaFlag(uint32 flag) const { return (m_staminaInfo.flags & flag) != 0; }
	void clearStaminaFlag(uint32 flag) { if (hasStaminaFlag(flag)) m_staminaInfo.flags &= ~flag; }
	uint32 getStaminaFlags() const { return m_staminaInfo.flags; }
	// 最大体力值
	void setMaxStamina(int32 maxStamina) { m_staminaInfo.maxStamina = maxStamina; }
	int32 getMaxStamina() const { return m_staminaInfo.maxStamina; }
	void setBaseMaxStamina(int32 stamina) { m_baseMaxStamina = stamina; }
	int32 getBaseMaxStamina() const { return m_baseMaxStamina; }
	// 蓄力开始的体力
	void setChargeStartStamina(int32 stamina) { m_staminaInfo.chargeStartStamina = stamina; }
	int32 getChargeStartStamina() const { return m_staminaInfo.chargeStartStamina; }
	// 已蓄力的体力
	void setChargedStamina(int32 stamina) { m_staminaInfo.chargedStamina = stamina; }
	int32 getChargedStamina() const { return m_staminaInfo.chargedStamina; }
	// 体力恢复比率。单位：体力恢复的比率/second
	void setStaminaRegenRate(float rate) { m_staminaInfo.staminaRegenRate = rate; }
	float getStaminaRegenRate() const { return m_staminaInfo.staminaRegenRate; }
	void setBaseStaminaRegenRate(float rate) { m_baseStaminaRegenRate = rate; }
	float getBaseStaminaRegenRate() const { return m_baseStaminaRegenRate; }
	// 每次攻击需要的体力
	void setAttackTakesStamina(int32 stamina) { m_attackTakesStamina = stamina; }
	int32 getAttackTakesStamina() const { return m_attackTakesStamina; }
	void setBaseAttackTakesStamina(int32 stamina) { m_baseAttackTakesStamina = stamina; }
	int32 getBaseAttackTakesStamina() const { return m_baseAttackTakesStamina; }
	// 蓄力攻击每秒消耗的体力
	void setChargeConsumesStamina(int32 stamina) { m_staminaInfo.chargeConsumesStamina = stamina; }
	int32 getChargeConsumesStamina() const { return m_staminaInfo.chargeConsumesStamina; }
	void setBaseChargeConsumesStamina(int32 stamina) { m_baseChargeConsumesStamina = stamina; }
	int32 getBaseChargeConsumesStamina() const { return m_baseChargeConsumesStamina; }
	// 攻击信息计数器
	void setAttackInfoCounter(uint32 counter) { m_staminaInfo.attackInfoCounter = counter; }
	uint32 getAttackInfoCounter() const { return m_staminaInfo.attackInfoCounter; }

	// 每次攻击造成的伤害
	void setDamage(int32 damage) { m_damage = damage; }
	int32 getDamage() const { return m_damage; }
	void setBaseDamage(int32 damage) { m_baseDamage = damage; }
	int32 getBaseDamage() const { return m_baseDamage; }

	// 防御力
	void setDefense(int32 defense) { m_defense = defense; }
	int32 getDefense() const { return m_defense; }

	// 战斗力
	uint16 getCombatPower() const { return m_combatPower; }
	void setCombatPower(uint16 cp) { m_combatPower = cp; }

	// void setMovementInfo(MovementInfo const& movement) { m_movementInfo = movement; }
	MovementInfo const& getMovementInfo() const { return m_movementInfo; }

	// 动作计数器
	// 默认情况下客户端与服务器的动作计数保持同步。
	// 在某些情况下（例如：重生、传输），需要将已同步到客户端的动作计数标记为无效，这就需要提高动作计数器，这样服务器将忽略来自客户端的与当前计数不匹配的动作
	void increaseMovementCounter() { ++m_movementInfo.counter; }
	uint32 getMovementCounter() const { return m_movementInfo.counter; }

	MovementInfo const& getMoveSegment() const { return m_moveSegment; }
	void markMoveSegment(MovementInfo const& movement) { m_moveSegment = movement; }
	void clearMoveSegment() { m_moveSegment.Clear(); }

	void setMovementFlags(uint32 flags) { m_movementInfo.flags = flags; }
	void addMovementFlag(uint32 flag) { if (!this->hasMovementFlag(flag)) m_movementInfo.flags |= flag; }
	bool hasMovementFlag(uint32 flag) const { return (m_movementInfo.flags & flag) != 0; }
	void clearMovementFlag(uint32 flag) { if (hasMovementFlag(flag)) m_movementInfo.flags &= ~flag; }
	uint32 getMovementFlags() const { return m_movementInfo.flags; }

	virtual void setMoney(int32 money) { m_money = money; }
	int32 getMoney() const { return m_money; }

	void addUnitFlag(uint32 flag);
	void clearUnitFlag(uint32 flag);
	bool hasUnitFlag(uint32 flag) const;	
	void setUnitFlags(uint32 flags);
	uint32 getUnitFlags() const { return m_unitFlags; }
	
	// 表情
	void setSmiley(uint16 code);
	uint16 getSmiley() const { return m_smiley; }

	// 隐蔽状态
	virtual void setConcealmentState(ConcealmentState state) { m_concealmentState = state; }
	ConcealmentState getConcealmentState() const { return m_concealmentState; }

	// 捡拾持续时间。单位：毫秒
	virtual void setPickupDuration(int32 duration) { m_pickupDuration = duration; }
	int32 getPickupDuration() const { return m_pickupDuration; }

	// 魔豆数量
	void setMagicBeanCount(int32 count);
	int32 getMagicBeanCount() const { return m_magicBeanCount; }

	// 击败人数
	virtual void setKillCount(int32 count) { m_killCount = count; }
	int32 getKillCount() const { return m_killCount; }

protected:
	MovementInfo m_movementInfo;
	MovementInfo m_moveSegment;

	std::string m_name;
	uint32 m_displayId;
	std::string m_country;
	TileCoord m_spawnPoint;

	int32 m_health;
	int32 m_maxHealth;
	int32 m_baseMaxHealth;
	float m_healthRegenRate;
	float m_baseHealthRegenRate;

	int32 m_moveSpeed;
	int32 m_baseMoveSpeed;
	float m_attackRange;
	float m_baseAttackRange;
	int32 m_damage;
	int32 m_baseDamage;
	int32 m_defense;

	StaminaInfo m_staminaInfo;
	int32 m_attackTakesStamina;
	int32 m_baseAttackTakesStamina;
	int32 m_baseMaxStamina;
	float m_baseStaminaRegenRate;
	int32 m_baseChargeConsumesStamina;

	uint8 m_level;
	int32 m_experience;
	int32 m_nextLevelXP;

	Point m_launchCenter;
	float m_launchRadiusInMap;

	int32 m_money;
	uint16 m_combatPower;
	uint32 m_unitFlags;
	uint16 m_smiley;
	ConcealmentState m_concealmentState;
	int32 m_pickupDuration;
	int32 m_magicBeanCount;
	int32 m_killCount;
};

#endif //__DATA_UNIT_H__
