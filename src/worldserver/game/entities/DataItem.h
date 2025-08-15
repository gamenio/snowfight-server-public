#ifndef __DATA_ITEM_H__
#define __DATA_ITEM_H__

#include "Common.h"
#include "DataWorldObject.h"

#define ITEM_PARABOLA_DURATION					500		// Duration of parabola motion of an item. Unit: milliseconds

// The number of items that can be stacked
#define ITEM_STACK_UNLIMITED					-1		// Unlimited
#define ITEM_STACK_NON_STACKABLE				0		// Non-stackable

#define MAGIC_BEAN_DROPPED_MAX_STACK_COUNT		3		// The maximum stack size for dropped magic beans
#define EQUIPMENT_LEVEL_MAX						3		// The maximum level of equipment

enum PickupStatus
{
	PICKUP_STATUS_OK,
	PICKUP_STATUS_ITEM_CANT_STACK,
	PICKUP_STATUS_ITEM_STACK_LIMIT_EXCEEDED,
	PICKUP_STATUS_INVENTORY_CUSTOM_SLOTS_FULL,
	PICKUP_STATUS_ITEM_IS_EQUIPPED,
	PICKUP_STATUS_LEVEL_LOWER_THAN_EXISTING_EQUIP,
	PICKUP_STATUS_FORBIDDEN,
};

enum ItemClass
{
	ITEM_CLASS_MAGIC_BEAN						= 0,
	ITEM_CLASS_EQUIPMENT						= 1,
	ITEM_CLASS_CONSUMABLE						= 2,
	ITEM_CLASS_GOLD								= 3,
};

enum ItemSubClassMagicBean
{
	ITEM_SUBCLASS_MAGIC_BEAN_SPARRING			= 0,
	ITEM_SUBCLASS_MAGIC_BEAN_TRAINING			= 1,
};

enum ItemSubClassEquipment
{
	ITEM_SUBCLASS_HAT							= 0,
	ITEM_SUBCLASS_JACKET						= 1,
	ITEM_SUBCLASS_GLOVES						= 2,
	ITEM_SUBCLASS_SNOWBALL_MAKER				= 3,
	ITEM_SUBCLASS_SHOES							= 4,
};

enum ItemSubClassConsumable
{
	ITEM_SUBCLASS_FIRST_AID						= 0,
	ITEM_SUBCLASS_CONSUMABLE_OTHER				= 1,
};

#define ITEM_APPLICATION_NONE					0
#define ITEM_VISUAL_NONE						0

enum ItemEffectType
{
	ITEM_EFFECT_NONE							= 0,
	ITEM_EFFECT_APPLY_STATS						= 1,
	ITEM_EFFECT_DISCOVER_CONCEALED_UNIT			= 2,
	ITEM_EFFECT_DAMAGE_REDUCTION_PERCENT		= 3,
	ITEM_EFFECT_INCREASE_HEALTH_PERCENT			= 4,
	ITEM_EFFECT_PREVENT_MOVE_SPEED_SLOWED		= 5,
	ITEM_EFFECT_CHARGED_ATTACK_ENABLED			= 6,
	ITEM_EFFECT_ENHANCE_PROJECTILE				= 7,
	ITEM_EFFECT_DAMAGE_BONUS_PERCENT			= 8,
	ITEM_EFFECT_INCREASE_HEALTH					= 9,
};

enum ItemApplicationFlag
{
	ITEM_APPFLAG_NONE							= 0,
	ITEM_APPFLAG_VISIBLE_TO_SELF				= 1 << 0,	// Visible to self or visible to all
};

enum ItemStatType
{
	ITEM_STAT_NONE								= 0,
	ITEM_STAT_MAX_HEALTH						= 1,
	ITEM_STAT_MAX_HEALTH_PERCENT				= 2,
	ITEM_STAT_HEALTH_REGEN_RATE					= 3,
	ITEM_STAT_HEALTH_REGEN_RATE_PERCENT			= 4,
	ITEM_STAT_ATTACK_RANGE						= 5,
	ITEM_STAT_ATTACK_RANGE_PERCENT				= 6,
	ITEM_STAT_MOVE_SPEED						= 7,
	ITEM_STAT_MOVE_SPEED_PERCENT				= 8,
	ITEM_STAT_MAX_STAMINA						= 9,
	ITEM_STAT_MAX_STAMINA_PERCENT				= 10,
	ITEM_STAT_STAMINA_REGEN_RATE				= 11,
	ITEM_STAT_STAMINA_REGEN_RATE_PERCENT		= 12,
	ITEM_STAT_DAMAGE							= 13,
	ITEM_STAT_DAMAGE_PERCENT					= 14,
	ITEM_STAT_DEFENSE							= 15,
};

struct ItemStat
{
	uint32 type;
	int32 value;
};

#define MAX_ITEM_STATS				2

struct ItemTemplate
{
	uint32 id;									// Item template ID. The value corresponds to ItemID
	uint32 itemClass;							// Item class
	uint32 displayId;							// Display ID
	uint32 itemSubClass;						// Item subclass
	int32 stackable;							// Number of stacks
	uint8 level;								// Item level
	uint32 appId;								// Item application ID
	std::vector<ItemStat> itemStats;			// List of statistics that apply to character
};

struct ItemSpawnInfo
{
	uint32 id;
	uint32 itemId;
	int32 minCount;
	int32 maxCount;
	TileCoord spawnPos;
	int32 spawnDist;
	int32 spawnTime;					// Spawning interval. Unit: seconds
};

struct LootItem
{
	uint32 itemId;
	float chance;
	int32 minCount;
	int32 maxCount;
};

struct ItemEffect
{
	uint32 type;
	int32 value;
};

#define MAX_ITEM_EFFECTS			2

// Item application duration type
#define ITEM_APP_DURATION_INSTANT				0		// Instant
#define ITEM_APP_DURATION_PERMANENT				-1		// Permanent

struct ItemApplicationTemplate
{
	uint32 id;									// Item application template ID
	uint32 flags;								// Item application flags
	uint32 visualId;							// Visual effect ID
	int32 duration;								// Duration. Unit: seconds
	int32 recoveryTime;							// Recovery time. Unit: seconds
	std::vector<ItemEffect> effects;			// Item effect list
};

struct ItemDest
{
	int32 pos;
	int32 count;
};

class DataItem: public DataWorldObject
{
public:
	DataItem();
	virtual ~DataItem();
    
	void writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const override;
	void updateDataForPlayer(UpdateType updateType, Player* player) override;

	uint32 getItemId() const { return m_itemId; }
	void setItemId(uint32 id) { m_itemId = id; }

	Point const& getPosition() const override { return m_position; }
	void setPosition(Point const& position)  override { m_position = position; }

	int32 getCount() const { return m_count; }
	void setCount(int32 count) { m_count = count; }

	ObjectGuid const& getHolder() const { return m_holder; }
	void setHolder(ObjectGuid const& guid) { m_holder = guid; }

	Point const& getHolderOrigin() const { return m_holderOrigin; }
	void setHolderOrigin(Point origin) { m_holderOrigin = origin; }

	Point const& getLaunchCenter() const { return m_launchCenter; }
	void setLaunchCenter(Point const& center) { m_launchCenter = center; }
	float getLaunchRadiusInMap() const { return m_launchRadiusInMap; };
	void setLaunchRadiusInMap(float radius) { m_launchRadiusInMap = radius; }

	// Drop duration. Unit: milliseconds
	int32 getDropDuration() const { return m_dropDuration; }
	void setDropDuration(int32 duration) { m_dropDuration = duration; }

	// Drop elapsed time. Unit: milliseconds
	int32 getDropElapsed() const { return m_dropElapsed; }
	void setDropElapsed(int32 elapsed) { m_dropElapsed = elapsed; }

private:
	uint32 m_itemId;
	Point m_position;
	int32 m_count;

	ObjectGuid m_holder;
	Point m_holderOrigin;
	Point m_launchCenter;
	float m_launchRadiusInMap;

	int32 m_dropDuration;
	int32 m_dropElapsed;
};



#endif // __DATA_ITEM_H__