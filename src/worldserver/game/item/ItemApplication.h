#ifndef __ITEM_APPLICATION_H__
#define __ITEM_APPLICATION_H__

#include "game/server/protocol/pb/ItemApplicationUpdate.pb.h"

#include "Common.h"
#include "utilities/Timer.h"
#include "game/behaviors/Item.h"

class Unit;

class ItemApplication
{
public:
	ItemApplication(ItemTemplate const* tmpl, Unit* target);
	~ItemApplication();

	ItemTemplate const* getTemplate() const { return m_template; }
	Unit* getTarget() const { return m_target; }

	void execute();
	void finish();

	bool update(NSTime diff);

	void sendApplicationUpdate(bool apply);
	void buildApplicationUpdate(ItemApplicationInfo& info, bool apply);

	bool isVisibleToSelf() const { return (m_appTemplate->flags & ITEM_APPFLAG_VISIBLE_TO_SELF) != 0; }
	int32 getDuration() const { return m_appTemplate->duration; }
	bool hasVisualEffect() const { return m_appTemplate->visualId != ITEM_VISUAL_NONE; }

	void effectApplyStats(ItemEffect const& effect, bool apply);
	void effectIncreaseHealthPercent(ItemEffect const& effect, bool apply);
	void effectDiscoverConcealedUnit(ItemEffect const& effect, bool apply);
	void effectChargedAttackEnabled(ItemEffect const& effect, bool apply);
	void effectRegisterToTarget(ItemEffect const& effect, bool apply);
	void effectIncreaseHealth(ItemEffect const& effect, bool apply);

private:
	void sendItemApplicationUpdateToPlayer(Player* player, ItemApplicationUpdate const& update);

	ItemTemplate const* m_template;
	ItemApplicationTemplate const* m_appTemplate;
	Unit* m_target;
	NSTime m_duration;
	DelayTimer m_finishTimer;
};
 
#endif // __ITEM_APPLICATION_H__