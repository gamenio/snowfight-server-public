#ifndef __DATA_ITEM_BOX_H__
#define __DATA_ITEM_BOX_H__

#include "Common.h"
#include "DataWorldObject.h"
#include "DataItem.h"

struct ItemBoxTemplate
{
	uint32 id;
	int32 maxHealth;
};

struct ItemBoxSpawnInfo
{
	uint32 templateId;
	uint32 lootId;
};

struct ItemBoxDistribution
{
	TileArea spawnArea;
};

struct ItemBoxLootTemplate
{
	uint32 id;
	std::vector<LootItem> itemList;
};

class DataItemBox : public DataWorldObject
{
public:
	enum Direction
	{
		RIGHT_DOWN,
		LEFT_DOWN,
	};

	DataItemBox();
	virtual ~DataItemBox();

	Point const& getLaunchCenter() const { return m_launchCenter; }
	void setLaunchCenter(Point const& center) { m_launchCenter = center; }

	Point const& getPosition() const override { return m_position; }
	void setPosition(Point const& position)  override { m_position = position; }

	uint8 getDirection() const { return m_direction; }
	void setDirection(uint8 dir) { m_direction = dir; }

	void setSpawnPoint(TileCoord const& spawnPoint) { m_spawnPoint = spawnPoint; }
	TileCoord const& getSpawnPoint() const { return m_spawnPoint; }

	int32 getHealth() const { return m_health; }
	void setHealth(int32 health);

	int32 getMaxHealth() const { return m_maxHealth; }
	void setMaxHealth(int32 maxHealth);
    
	void writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const override;

private:
	Point m_launchCenter;
	Point m_position;
	uint8 m_direction;
	TileCoord m_spawnPoint;

	int32 m_health;
	int32 m_maxHealth;
};

#endif // __DATA_ITEM_BOX_H__