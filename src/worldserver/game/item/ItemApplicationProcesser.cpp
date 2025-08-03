#include "ItemApplicationProcesser.h"

#include "game/server/protocol/pb/ItemApplicationUpdate.pb.h"

#include "game/server/WorldSession.h"
#include "game/behaviors/Player.h"

ItemApplicationProcesser::ItemApplicationProcesser(Unit* owner) :
	m_owner(owner)
{
}

ItemApplicationProcesser::~ItemApplicationProcesser()
{
	m_owner = nullptr;
}

void ItemApplicationProcesser::applyItem(ItemTemplate const* tmpl)
{
	if (tmpl->appId == ITEM_APPLICATION_NONE)
		return;

	this->unapplyItem(tmpl);

	ItemApplication app(tmpl, m_owner);
	if (app.getDuration() > 0 || app.getDuration() == ITEM_APP_DURATION_PERMANENT)
		m_applications.emplace(tmpl->appId, app);
	app.execute();
}

void ItemApplicationProcesser::unapplyItem(ItemTemplate const* tmpl)
{
	auto it = m_applications.find(tmpl->appId);
	if (it != m_applications.end())
	{
		ItemApplication& app = (*it).second;
		app.finish();
		m_applications.erase(it);
	}
}

void ItemApplicationProcesser::removeAll()
{
	for (auto it = m_applications.begin(); it != m_applications.end(); )
	{
		ItemApplication& app = (*it).second;
		app.finish();
		it = m_applications.erase(it);
	}
}

void ItemApplicationProcesser::sendApplicationUpdateAllToPlayer(Player* player, bool apply)
{
	if (!player->getSession())
		return;

	ItemApplicationUpdateAll updateAll;
	updateAll.set_target(m_owner->getData()->getGuid().getRawValue());

	for (auto it = m_applications.begin(); it != m_applications.end(); ++it)
	{
		ItemApplication& app = (*it).second;
		if (app.hasVisualEffect())
		{
			if (!app.isVisibleToSelf() || m_owner == player)
				app.buildApplicationUpdate(*updateAll.add_app_list(), apply);
		}
	}

	if (!updateAll.app_list().empty())
	{
		WorldPacket packet(SMSG_ITEM_APPLICATION_UPDATE_ALL);
		player->getSession()->packAndSend(std::move(packet), updateAll);
	}
}

void ItemApplicationProcesser::update(NSTime diff)
{
	for (auto it = m_applications.begin(); it != m_applications.end();)
	{
		ItemApplication& app = (*it).second;
		if (!app.update(diff))
			it = m_applications.erase(it);
		else
			++it;
	}
}
