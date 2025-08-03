#ifndef __ITEM_BOX_H__
#define __ITEM_BOX_H__

#include "game/movement/generators/FollowerRefManager.h"
#include "game/movement/generators/FollowerReference.h"
#include "game/entities/DataItemBox.h"
#include "WorldObject.h"

struct SimpleItemBox
{
	SimpleItemBox() :
		templateId(0),
		spawnPoint(TileCoord::INVALID),
		direction(DataItemBox::LEFT_DOWN),
		lootId(0)
	{}

	uint32 templateId;
	TileCoord spawnPoint;
	uint8 direction;
	uint32 lootId;
};

struct ItemBoxItem
{
	uint32 itemId;
	int32 count;
};

enum OpenState
{
	OPEN_STATE_LOCKED,
	OPEN_STATE_OPENED,
};

class ItemBox : public AttackableObject, public GridObject<ItemBox>
{
public:
	ItemBox();
    virtual ~ItemBox();

	void update(NSTime diff) override;
	void addToWorld() override;
	void removeFromWorld() override;

	void cleanupBeforeDelete() override;

	void addUnlocker(Unit* unlocker);
	void removeUnlocker(Unit* unlocker);
	void removeAllUnlockers();
	int32 getUnlockerCount() const { return (int32)m_unlockers.size(); }
	bool isUnlocker(Unit* unit) const;

	void enterCollision(Projectile* proj, float precision) override;
	void receiveDamage(Unit* attacker, int32 damage);

	bool isLocked() const { return m_openState == OPEN_STATE_LOCKED; }
	void opened(Unit* opener);

	FollowerRefManager<ItemBox>* getFollowerRefManager() { return &m_followerRefManager; }

	bool loadData(SimpleItemBox const& simpleItemBox, BattleMap* map);
	bool reloadData(SimpleItemBox const& simpleItemBox, BattleMap* map);
	virtual DataItemBox* getData() const override { return static_cast<DataItemBox*>(m_data); }

private:
	void setOpenState(OpenState state);

	void dropLoot(Unit* looter);
	void createItem(ItemTemplate const* tmpl, TileCoord const& spawnPoint, int32 count);

	OpenState m_openState;

	uint32 m_templateId;
	uint32 m_lootId;
	std::unordered_set<Unit*> m_unlockers;
	FollowerRefManager<ItemBox> m_followerRefManager;
};



#endif // __ITEM_BOX_H__
