#ifndef __INTERESTED_REFERENCE_H__
#define __INTERESTED_REFERENCE_H__

#include "game/dynamic/reference/Reference.h"
#include "game/entities/ObjectGuid.h"

class Item;
class WishManager;

class InterestedReference: public Reference<Item, WishManager>
{
	enum GoldStackSize
	{
		GOLD_STACK_TINY,
		GOLD_STACK_SMALL,
		GOLD_STACK_MEDIUM,
		GOLD_STACK_LARGE,
		GOLD_STACK_HUGE,
		MAX_GOLD_STACKS
	};

	enum MagicBeanStyle
	{
		MAGICBEAN_SINGLE,
		MAGICBEAN_STACKED,
		MAX_MAGICBEAN_STYLES
	};

public:
	InterestedReference(Item* item, WishManager* wishManager);
	~InterestedReference();

	void update();

	ObjectGuid getTargetGuid() const { return m_targetGuid; }
	float getWish() const { return m_wish; }

	InterestedReference* next() { return ((InterestedReference*)Reference<Item, WishManager>::next()); }

protected:
	void buildLink() override;
	void destroyLink() override;

private:
	GoldStackSize getGoldStackSize(int32 golds) const;
	MagicBeanStyle getMagicBeanStyle(int32 count) const;

	ObjectGuid m_targetGuid;
	float m_wish;
};

#endif // __INTERESTED_REFERENCE_H__