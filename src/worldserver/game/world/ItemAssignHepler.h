#ifndef __ITEM_ASSIGN_HEPLER_H__
#define __ITEM_ASSIGN_HEPLER_H__

#include "Common.h"
#include "game/entities/DataItemBox.h"

class SortedItemBox
{
public:
	SortedItemBox();
	~SortedItemBox();

	void addItem(LootItem const& item);

	int32 getItemCount() const { return (int32)m_itemLookupTable.size(); }
	int32 getItemCount(uint32 itemId) const{ return (int32)m_itemLookupTable.count(itemId); }
	int32 getClassItemCount(uint32 itemClass) const;
	int32 getSubclassItemCount(uint32 itemClass, uint32 itemSubClass) const;

	std::list<LootItem> const& getItemList() const { return m_itemList; }

private:
	std::size_t getItemClassHash(uint32 itemClass, uint32 itemSubClasss) const;

	std::list<LootItem> m_itemList;
	std::unordered_multimap<uint32/* ItemId */, std::list<LootItem>::iterator> m_itemLookupTable;
	std::unordered_multimap<uint32/*  ItemClass */, std::list<LootItem>::iterator> m_classItemLookupTable;
	std::unordered_multimap<std::size_t/* ItemClassHash */, std::list<LootItem>::iterator> m_subclassItemLookupTable;
};

int32 assignItemsToItemBoxs(std::vector<LootItem>& itemList, std::vector<SortedItemBox>& itemBoxList);

#endif // __ITEM_ASSIGN_HEPLER_H__