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

// Concealment state
enum ConcealmentState
{
	CONCEALMENT_STATE_EXPOSED,
	CONCEALMENT_STATE_CONCEALING,
	CONCEALMENT_STATE_CONCEALED,
};

// Hero character ID
// Same as player and robot template ID
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

#define	MAX_ATTACK_RANGE				343.0f	// Maximum attack range. Unit: points
#define MAX_MOVE_SPEED					150		// Maximum moving speed. Unit: points/second

// Unit level range
#define LEVEL_MIN			1
#define LEVEL_MAX			200

// Maximum combat power
#define COMBAT_POWER_MAX	1000

// Default country
#define COUNTRY_DEFAULT		"US"

// Smiley code
enum SmileyCode
{
	SMILEY_NONE				= 0,
	SMILEY_LAUGH			= 1,
	SMILEY_NAUGHTY			= 2,
	SMILEY_DEVIL			= 3,
	SMILEY_SAD				= 4,
	SMILEY_CRY				= 5,
	SMILEY_ANGRY			= 6,
};

enum UnitFlag: uint32
{
	UNIT_FLAG_NONE				= 0,
	UNIT_FLAG_DEATH_LOSE_MONEY	= 1 << 0,	// Whether money is lost when a unit dies (health reaches 0)
	UNIT_FLAG_DAMAGED			= 1 << 1,	// Unit received damage
}; 

struct CombatGrade
{
	uint8 grade;
	uint16 minCombatPower;
	uint16 maxCombatPower;
	float loseMoneyPercent;
	float loseMoneyApportionment;
	float robotNoNameChance;
	int32 dangerStateHealthLoss;	// The health points lost per second when the unit is in a danger zone.
};

#define TRAINING_GROUND_COMBAT_GRADE					0	// Combat grade of the training ground

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

// Statistic indices that measure combat power
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

	// Name
	void setName(std::string const& name) { m_name = name; }
	std::string const& getName() const { return m_name; }

	// Display ID
	uint32 getDisplayId() const { return m_displayId; }
	void setDisplayId(uint32 id) { m_displayId = id; }

	 // Country
	void setCountry(std::string const& country) { m_country = country; }
	std::string const& getCountry() const { return m_country; }

	// Spawning point
	void setSpawnPoint(TileCoord const& spawnPoint) { m_spawnPoint = spawnPoint; }
	TileCoord const& getSpawnPoint() const { return m_spawnPoint; }

	// Level
	void setLevel(uint8 level);
	uint8 getLevel() const { return m_level; }
	// Experience
	virtual void setExperience(int32 xp) { m_experience = xp; }
	int32 getExperience() const { return m_experience; }
	// Experience required for the next level
	virtual void setNextLevelXP(int32 xp) { m_nextLevelXP = xp; }
	int32 getNextLevelXP() const { return m_nextLevelXP; }

	float getOrientation() const { return m_movementInfo.orientation; }
	void setOrientation(float rad);
	void randomOrientation() { this->setOrientation(float(random((float)-M_PI, (float)M_PI)));}

	// Attack range. Unit: points
	void setAttackRange(float range);
	float getAttackRange() const { return m_attackRange; }
	void setBaseAttackRange(float range) { m_baseAttackRange = range; }
	float getBaseAttackRange() const { return m_baseAttackRange; }

	// Moving speed. Unit: points/second
	int32 getMoveSpeed() const { return m_moveSpeed; }
	void setMoveSpeed(int32 moveSpeed);
	int32 getBaseMoveSpeed() const { return m_baseMoveSpeed; }
	void setBaseMoveSpeed(int32 moveSpeed) { m_baseMoveSpeed = moveSpeed; }

	// Health
	int32 getHealth() const { return m_health; }
	void setHealth(int32 health);
	// Maximum health
	int32 getMaxHealth() const { return m_maxHealth; }
	void setMaxHealth(int32 maxHealth);
	int32 getBaseMaxHealth() const { return m_baseMaxHealth; }
	void setBaseMaxHealth(int32 maxHealth) { m_baseMaxHealth = maxHealth; }
	// Health regeneration rate. Unit: Health regeneration rate/minute
	void setHealthRegenRate(float rate) { m_healthRegenRate = rate; }
	float getHealthRegenRate() const { return m_healthRegenRate; }
	void setBaseHealthRegenRate(float rate) { m_baseHealthRegenRate = rate; }
	float getBaseHealthRegenRate() const { return m_baseHealthRegenRate; }

	// Stamina
	void setStamina(int32 stamina) { m_staminaInfo.stamina = stamina; }
	int32 getStamina() const { return m_staminaInfo.stamina; }
	StaminaInfo const& getStaminaInfo() const { return m_staminaInfo; }
	// Stamina counter
	void increaseStaminaCounter() { ++m_staminaInfo.counter; }
	uint32 getStaminaCounter() const { return m_staminaInfo.counter; }
	// Attack counter
	uint32 getAttackCounter() const { return m_staminaInfo.attackCounter; }
	void increaseAttackCounter() { ++m_staminaInfo.attackCounter; }
	void resetAttackCounter() { m_staminaInfo.attackCounter = 0; }
	//  The total of stamina consumed.
	uint32 getConsumedStaminaTotal() const { return m_staminaInfo.consumedStaminaTotal; }
	void increaseConsumedStaminaTotal(uint32 amount) { m_staminaInfo.consumedStaminaTotal += amount; }
	void resetConsumedStaminaTotal() { m_staminaInfo.consumedStaminaTotal = 0; }
	// Stamina flag
	void setStaminaFlags(uint32 flags) { m_staminaInfo.flags = flags; }
	void addStaminaFlag(uint32 flag) { if (!this->hasStaminaFlag(flag)) m_staminaInfo.flags |= flag; }
	bool hasStaminaFlag(uint32 flag) const { return (m_staminaInfo.flags & flag) != 0; }
	void clearStaminaFlag(uint32 flag) { if (hasStaminaFlag(flag)) m_staminaInfo.flags &= ~flag; }
	uint32 getStaminaFlags() const { return m_staminaInfo.flags; }
	// Maximum stamina
	void setMaxStamina(int32 maxStamina) { m_staminaInfo.maxStamina = maxStamina; }
	int32 getMaxStamina() const { return m_staminaInfo.maxStamina; }
	void setBaseMaxStamina(int32 stamina) { m_baseMaxStamina = stamina; }
	int32 getBaseMaxStamina() const { return m_baseMaxStamina; }
	// Stamina at the start of charging
	void setChargeStartStamina(int32 stamina) { m_staminaInfo.chargeStartStamina = stamina; }
	int32 getChargeStartStamina() const { return m_staminaInfo.chargeStartStamina; }
	// Charged stamina
	void setChargedStamina(int32 stamina) { m_staminaInfo.chargedStamina = stamina; }
	int32 getChargedStamina() const { return m_staminaInfo.chargedStamina; }
	// Stamina regeneration rate. Unit: Stamina regeneration rate / second
	void setStaminaRegenRate(float rate) { m_staminaInfo.staminaRegenRate = rate; }
	float getStaminaRegenRate() const { return m_staminaInfo.staminaRegenRate; }
	void setBaseStaminaRegenRate(float rate) { m_baseStaminaRegenRate = rate; }
	float getBaseStaminaRegenRate() const { return m_baseStaminaRegenRate; }
	// The stamina required for each attack
	void setAttackTakesStamina(int32 stamina) { m_attackTakesStamina = stamina; }
	int32 getAttackTakesStamina() const { return m_attackTakesStamina; }
	void setBaseAttackTakesStamina(int32 stamina) { m_baseAttackTakesStamina = stamina; }
	int32 getBaseAttackTakesStamina() const { return m_baseAttackTakesStamina; }
	// Stamina consumed per second for charged attack
	void setChargeConsumesStamina(int32 stamina) { m_staminaInfo.chargeConsumesStamina = stamina; }
	int32 getChargeConsumesStamina() const { return m_staminaInfo.chargeConsumesStamina; }
	void setBaseChargeConsumesStamina(int32 stamina) { m_baseChargeConsumesStamina = stamina; }
	int32 getBaseChargeConsumesStamina() const { return m_baseChargeConsumesStamina; }
	// Attack information counter
	void setAttackInfoCounter(uint32 counter) { m_staminaInfo.attackInfoCounter = counter; }
	uint32 getAttackInfoCounter() const { return m_staminaInfo.attackInfoCounter; }

	// Damage dealing by each attack
	void setDamage(int32 damage) { m_damage = damage; }
	int32 getDamage() const { return m_damage; }
	void setBaseDamage(int32 damage) { m_baseDamage = damage; }
	int32 getBaseDamage() const { return m_baseDamage; }

	// Defense
	void setDefense(int32 defense) { m_defense = defense; }
	int32 getDefense() const { return m_defense; }

	// Combat power
	uint16 getCombatPower() const { return m_combatPower; }
	void setCombatPower(uint16 cp) { m_combatPower = cp; }

	// void setMovementInfo(MovementInfo const& movement) { m_movementInfo = movement; }
	MovementInfo const& getMovementInfo() const { return m_movementInfo; }

	// Movement counter
	// By default, the movement counter is synchronized between the client and the server.
	// In some cases (e.g., respawn, transport) it is necessary to invalidate the movement counter synchronized to the client. 
	// This requires increasing the movement counter so that the server ignores movement from the client that does not match the current count.
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
	
	// Smiley
	void setSmiley(uint16 code);
	uint16 getSmiley() const { return m_smiley; }

	// Concealment state
	virtual void setConcealmentState(ConcealmentState state) { m_concealmentState = state; }
	ConcealmentState getConcealmentState() const { return m_concealmentState; }

	// Pickup duration. Unit: milliseconds
	virtual void setPickupDuration(int32 duration) { m_pickupDuration = duration; }
	int32 getPickupDuration() const { return m_pickupDuration; }

	// Number of magic beans
	void setMagicBeanCount(int32 count);
	int32 getMagicBeanCount() const { return m_magicBeanCount; }

	// Number of kills
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

#endif // __DATA_UNIT_H__
