#ifndef __ITEM_COOLDOWN_PROCESSER_H__
#define __ITEM_COOLDOWN_PROCESSER_H__

#include "game/server/protocol/pb/ItemCooldownList.pb.h"

#include "Common.h"
#include "ItemCooldown.h"
#include "game/behaviors/CarriedItem.h"

class Unit;

class ItemCooldownProcesser
{
public:
	ItemCooldownProcesser(Unit* owner);
	~ItemCooldownProcesser();

	void startCooldown(CarriedItem* item);

	bool isReady(uint32 itemId) const;
	NSTime getRemainingCooldown(uint32 itemId) const;

	void removeAll() { m_cooldowns.clear(); }
	void update(NSTime diff);

	void buildItemCooldownList(ItemCooldownList& message);

private:
	Unit* m_owner;
	std::unordered_map<uint32/* ItemID */, ItemCooldown> m_cooldowns;
};

#endif // __ITEM_COOLDOWN_PROCESSER_H__
