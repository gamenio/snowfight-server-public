#include "ItemCooldown.h"

#include "game/world/ObjectMgr.h"


ItemCooldown::ItemCooldown(ItemTemplate const* tmpl)
{
	ItemApplicationTemplate const* appTmpl = sObjectMgr->getItemApplicationTemplate(tmpl->appId);
	NS_ASSERT(appTmpl);

	m_finishTimer.setDuration(appTmpl->recoveryTime * 1000);
}

ItemCooldown::~ItemCooldown()
{
}

bool ItemCooldown::update(NSTime diff)
{
	m_finishTimer.update(diff);
	if (m_finishTimer.passed())
	{
		m_finishTimer.reset();
		return false;
	}

	return true;
}