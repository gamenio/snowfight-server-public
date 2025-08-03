#include "InterestedReference.h"

#include "game/behaviors/Item.h"
#include "game/collection/WishManager.h"
#include "game/world/ObjectMgr.h"
#include "game/behaviors/Robot.h"

// 愿望的优先级
// 值越大优先级越高
enum WishPriority
{
	WISH_PRIORITY_NONE,
	WISH_PRIORITY_GOLD,
	WISH_PRIORITY_CONSUMABLE_OTHER,
	WISH_PRIORITY_MAGIC_BEAN,
	WISH_PRIORITY_EQUIPMENT,
	WISH_PRIORITY_FIRST_AID
};

InterestedReference::InterestedReference(Item* item, WishManager* wishManager) :
	m_wish(0.f)
{
	this->link(item, wishManager);
}

InterestedReference::~InterestedReference()
{
}

void InterestedReference::update()
{
	Item* item = this->getTarget();
	Robot* collector = this->getSource()->getOwner();
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item->getData()->getItemId());
	NS_ASSERT(tmpl);

	int32 priority = WISH_PRIORITY_NONE;
	float normalize = 0.f;
	switch (tmpl->itemClass)
	{
	case ITEM_CLASS_CONSUMABLE:
	{
		if (tmpl->itemSubClass == ITEM_SUBCLASS_FIRST_AID)
			priority = WISH_PRIORITY_FIRST_AID;
		else
			priority = WISH_PRIORITY_CONSUMABLE_OTHER;
		//float dist = collector->getData()->getPosition().getDistance(item->getData()->getPosition());
		//normalize = std::min(1.0f, dist / collector->getSightDistance());
		normalize = 1.0f;
		break;
	}
	case ITEM_CLASS_EQUIPMENT:
		priority = WISH_PRIORITY_EQUIPMENT;
		normalize = (tmpl->level > 0 ? tmpl->level : EQUIPMENT_LEVEL_MAX) / (float)EQUIPMENT_LEVEL_MAX;
		break;
	case ITEM_CLASS_MAGIC_BEAN:
	{
		priority = WISH_PRIORITY_MAGIC_BEAN;
		int32 style = this->getMagicBeanStyle(item->getData()->getCount());
		normalize = (style + 1) / (float)MAX_MAGICBEAN_STYLES;
		break;
	}
	case ITEM_CLASS_GOLD:
	{
		priority = WISH_PRIORITY_GOLD;
		int32 size = this->getGoldStackSize(item->getData()->getCount());
		normalize = (size + 1) / (float)MAX_GOLD_STACKS;
		break;
	}
	default:
		break;
	}
	m_wish = priority + 0.9f * normalize;
}

void InterestedReference::buildLink()
{
	m_targetGuid = this->getTarget()->getData()->getGuid();
	this->getTarget()->getInterestedRefManager()->insertFirst(this);
	this->getTarget()->getInterestedRefManager()->incSize();
}

void InterestedReference::destroyLink()
{
	this->getTarget()->getInterestedRefManager()->decSize();
	this->getSource()->setDirty(true);
}

InterestedReference::GoldStackSize InterestedReference::getGoldStackSize(int32 golds) const
{
	if (golds < 20)
		return GOLD_STACK_TINY;
	else if (golds < 50)
		return GOLD_STACK_SMALL;
	else if (golds < 100)
		return GOLD_STACK_MEDIUM;
	else if (golds < 200)
		return GOLD_STACK_LARGE;
	else
		return GOLD_STACK_HUGE;
}

InterestedReference::MagicBeanStyle InterestedReference::getMagicBeanStyle(int32 count) const
{
	MagicBeanStyle style = MAGICBEAN_SINGLE;
	if (count > 1)
		style = MAGICBEAN_STACKED;

	return style;
}
