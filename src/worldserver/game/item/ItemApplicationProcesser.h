#ifndef __ITEM_APPLICATION_PROCESSER_H__
#define __ITEM_APPLICATION_PROCESSER_H__

#include "Common.h"
#include "ItemApplication.h"

class Unit;

class ItemApplicationProcesser
{
public:
	ItemApplicationProcesser(Unit* owner);
	~ItemApplicationProcesser();

	void applyItem(ItemTemplate const* tmpl);
	void unapplyItem(ItemTemplate const* tmpl);
	void removeAll();

	void sendApplicationUpdateAllToPlayer(Player* player, bool apply);

	void update(NSTime diff);

private:
	Unit* m_owner;
	std::unordered_map<uint32/* ApplicationID */, ItemApplication> m_applications;
};

#endif // __ITEM_APPLICATION_PROCESSER_H__
