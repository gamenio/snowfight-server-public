#include "ItemCooldownProcesser.h"

#include "game/world/ObjectMgr.h"
#include "game/behaviors/CarriedItem.h"


ItemCooldownProcesser::ItemCooldownProcesser(Unit* owner) :
	m_owner(owner)
{
}

ItemCooldownProcesser::~ItemCooldownProcesser()
{
	m_owner = nullptr;
}

void ItemCooldownProcesser::startCooldown(CarriedItem* item)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item->getData()->getItemId());
	NS_ASSERT(tmpl);

	if (tmpl->appId == ITEM_APPLICATION_NONE)
		return;

	ItemCooldown cooldown(tmpl);
	m_cooldowns.emplace(item->getData()->getItemId(), cooldown);
}

bool ItemCooldownProcesser::isReady(uint32 itemId) const
{
	auto it = m_cooldowns.find(itemId);
	return it == m_cooldowns.end();
}

NSTime ItemCooldownProcesser::getRemainingCooldown(uint32 itemId) const
{
	auto it = m_cooldowns.find(itemId);
	if (it != m_cooldowns.end())
		return (*it).second.getRemainingTime();

	return 0;
}

void ItemCooldownProcesser::update(NSTime diff)
{
	for (auto it = m_cooldowns.begin(); it != m_cooldowns.end();)
	{
		ItemCooldown& cooldown = (*it).second;
		if (!cooldown.update(diff))
			it = m_cooldowns.erase(it);
		else
			++it;
	}
}

void ItemCooldownProcesser::buildItemCooldownList(ItemCooldownList& message)
{
	for (auto it = m_cooldowns.begin(); it != m_cooldowns.end(); ++it)
	{
		uint32 itemId = (*it).first;
		ItemCooldown const& cooldown = (*it).second;

		ItemCooldownInfo* info = message.add_result();
		info->set_item_id(itemId);
		info->set_duration(cooldown.getDuration());
		info->set_remaining_time(cooldown.getRemainingTime());
	}
}
