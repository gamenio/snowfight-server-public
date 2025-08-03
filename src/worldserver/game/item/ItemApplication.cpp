#include "ItemApplication.h"

#include "game/behaviors/Unit.h"
#include "game/world/ObjectMgr.h"
#include "game/grids/ObjectSearcher.h"
#include "game/server/WorldSession.h"
#include "game/behaviors/Player.h"

typedef void (ItemApplication::*ItemEffectHandler)(ItemEffect const& effect, bool apply);

std::unordered_map<uint32 /* ItemEffectType */, ItemEffectHandler> gItemEffectTable =
{
	{ ITEM_EFFECT_APPLY_STATS,								&ItemApplication::effectApplyStats				},
	{ ITEM_EFFECT_DISCOVER_CONCEALED_UNIT,					&ItemApplication::effectDiscoverConcealedUnit	},
	{ ITEM_EFFECT_DAMAGE_REDUCTION_PERCENT,					&ItemApplication::effectRegisterToTarget		},
	{ ITEM_EFFECT_INCREASE_HEALTH_PERCENT,					&ItemApplication::effectIncreaseHealthPercent	},
	{ ITEM_EFFECT_PREVENT_MOVE_SPEED_SLOWED,				&ItemApplication::effectRegisterToTarget		},
	{ ITEM_EFFECT_CHARGED_ATTACK_ENABLED,					&ItemApplication::effectChargedAttackEnabled	},
	{ ITEM_EFFECT_ENHANCE_PROJECTILE,						&ItemApplication::effectRegisterToTarget		},
	{ ITEM_EFFECT_DAMAGE_BONUS_PERCENT,						&ItemApplication::effectRegisterToTarget		},
	{ ITEM_EFFECT_INCREASE_HEALTH,							&ItemApplication::effectIncreaseHealth			},
};


ItemApplication::ItemApplication(ItemTemplate const* tmpl, Unit* target) :
	m_template(tmpl),
	m_appTemplate(nullptr),
	m_target(target)
{
	m_appTemplate = sObjectMgr->getItemApplicationTemplate(m_template->appId);
	NS_ASSERT(m_appTemplate);

	m_finishTimer.setDuration(m_appTemplate->duration * 1000);
}

ItemApplication::~ItemApplication()
{
	m_template = nullptr;
	m_target = nullptr;
}

void ItemApplication::execute()
{
	for (auto const& effect: m_appTemplate->effects)
	{
		auto it = gItemEffectTable.find(effect.type);
		if (it != gItemEffectTable.end())
		{
			ItemEffectHandler handler = (*it).second;
			(this->*handler)(effect, true);
		}
		else
			NS_LOG_ERROR("item.application", "Unhandled item effect (type: %d)", effect.type);
	}

	this->sendApplicationUpdate(true);
}

void ItemApplication::finish()
{
	for (auto const& effect : m_appTemplate->effects)
	{
		auto it = gItemEffectTable.find(effect.type);
		if (it != gItemEffectTable.end())
		{
			ItemEffectHandler handler = (*it).second;
			(this->*handler)(effect, false);
		}
		else
			NS_LOG_ERROR("item.application", "Unhandled item effect (type: %d)", effect.type);
	}

	this->sendApplicationUpdate(false);
}

bool ItemApplication::update(NSTime diff)
{
	if (m_appTemplate->duration > 0)
	{
		m_finishTimer.update(diff);
		if (m_finishTimer.passed())
		{
			this->finish();
			m_finishTimer.reset();

			return false;
		}
	}

	return true;
}

void ItemApplication::sendApplicationUpdate(bool apply)
{
	if (!this->hasVisualEffect())
		return;

	ItemApplicationUpdate update;
	update.set_target(m_target->getData()->getGuid().getRawValue());
	this->buildApplicationUpdate(*update.mutable_app(), apply);

	Player* player = m_target->asPlayer();
	if (player)
		this->sendItemApplicationUpdateToPlayer(player, update);

	if (!this->isVisibleToSelf())
	{
		PlayerRangeExistsObjectFilter filter(m_target);
		std::list<Player*> result;
		ObjectSearcher<Player, PlayerRangeExistsObjectFilter> searcher(filter, result);

		m_target->visitNearbyObjectsInMaxVisibleRange(searcher);

		for (auto it = result.begin(); it != result.end(); ++it)
		{
			Player* player = *it;
			this->sendItemApplicationUpdateToPlayer(player, update);
		}
	}
}

void ItemApplication::buildApplicationUpdate(ItemApplicationInfo& info, bool apply)
{
	info.set_apply(apply);
	info.set_item_id(m_template->id);
	if (apply)
	{
		info.set_duration(m_finishTimer.getDuration());
		info.set_remaining_time(m_finishTimer.getRemainder());
	}
}

void ItemApplication::effectApplyStats(ItemEffect const& effect, bool apply)
{
	m_target->applyItemStats(m_template->itemStats, apply);
}

void ItemApplication::effectIncreaseHealthPercent(ItemEffect const& effect, bool apply)
{
	if (!apply)
		return;

	int32 health = m_target->getData()->getHealth();
	int32 addValue = (int32)((float)effect.value / 100.f * m_target->getData()->getMaxHealth());
	int32 newHealth = std::min(m_target->getData()->getMaxHealth(), health + addValue);
	m_target->getData()->setHealth(newHealth);
	m_target->getUnitHostileRefManager()->updateThreat(UNITTHREAT_ENEMY_HEALTH);
}

void ItemApplication::effectDiscoverConcealedUnit(ItemEffect const& effect, bool apply)
{
	if (apply)
		m_target->registerItemEffect(&effect);
	else
		m_target->unregisterItemEffect(&effect);
	m_target->updateObjectVisibility();
}

void ItemApplication::effectChargedAttackEnabled(ItemEffect const& effect, bool apply)
{
	if (apply)
		m_target->registerItemEffect(&effect);
	else
		m_target->unregisterItemEffect(&effect);
}

void ItemApplication::effectRegisterToTarget(ItemEffect const& effect, bool apply)
{
	if (apply)
		m_target->registerItemEffect(&effect);
	else
		m_target->unregisterItemEffect(&effect);
}

void ItemApplication::sendItemApplicationUpdateToPlayer(Player* player, ItemApplicationUpdate const& update)
{
	if (!player->getSession())
		return;

	WorldPacket packet(SMSG_ITEM_APPLICATION_UPDATE);
	player->getSession()->packAndSend(std::move(packet), update);
}

void ItemApplication::effectIncreaseHealth(ItemEffect const& effect, bool apply)
{
	if (!apply)
		return;

	int32 health = m_target->getData()->getHealth();
	int32 newHealth = std::min(m_target->getData()->getMaxHealth(), health + effect.value);
	m_target->getData()->setHealth(newHealth);
	m_target->getUnitHostileRefManager()->updateThreat(UNITTHREAT_ENEMY_HEALTH);
}