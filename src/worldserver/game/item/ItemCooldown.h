#ifndef __ITEM_COOLDOWN_H__
#define __ITEM_COOLDOWN_H__

#include "Common.h"
#include "game/behaviors/Item.h"

class Unit;

class ItemCooldown
{
public:
	ItemCooldown(ItemTemplate const* tmpl);
	~ItemCooldown();

	bool update(NSTime diff);

	NSTime getDuration() const { return m_finishTimer.getDuration(); }
	NSTime getRemainingTime() const { return m_finishTimer.getRemainder(); }

private:
	DelayTimer m_finishTimer;
};
 
#endif // __ITEM_COOLDOWN_H__