#include "ItemAssignHepler.h"

#include <boost/functional/hash/hash.hpp>

#include "game/world/ObjectMgr.h"

struct LootItemOrderPred
{
	bool operator()(LootItem const& a, LootItem const& b) const
	{
		if (a.chance != b.chance)
			return a.chance > b.chance;

		return a.itemId < b.itemId;
	}
};

struct ItemBoxOrderPred
{
	ItemBoxOrderPred(LootItem const& lootItem) :
		m_lootItem(lootItem),
		m_template(nullptr)
	{
		m_template = sObjectMgr->getItemTemplate(m_lootItem.itemId);
		NS_ASSERT(m_template);
	}

	bool operator()(SortedItemBox const& a, SortedItemBox const& b) const
	{
		auto aIdCount = a.getItemCount(m_lootItem.itemId);
		auto bIdCount = b.getItemCount(m_lootItem.itemId);
		if (aIdCount != bIdCount)
			return aIdCount < bIdCount;

		auto aClassCount = a.getClassItemCount(m_template->itemClass);
		auto bClassCount = b.getClassItemCount(m_template->itemClass);
		if (aClassCount != bClassCount)
			return aClassCount < bClassCount;

		auto aSubclassCount = a.getSubclassItemCount(m_template->itemClass, m_template->itemSubClass);
		auto bSubclassCount = b.getSubclassItemCount(m_template->itemClass, m_template->itemSubClass);
		if (aSubclassCount != bSubclassCount)
			return aSubclassCount < bSubclassCount;

		return a.getItemCount() < b.getItemCount();
	}

private:
	LootItem const& m_lootItem;
	ItemTemplate const* m_template;
};

SortedItemBox::SortedItemBox()
{
}

SortedItemBox::~SortedItemBox()
{
}

void SortedItemBox::addItem(LootItem const& item)
{
	auto it = m_itemList.emplace(m_itemList.end(), item);
	m_itemLookupTable.emplace(item.itemId, it);

	auto const* tmpl = sObjectMgr->getItemTemplate(item.itemId);
	m_classItemLookupTable.emplace(tmpl->itemClass, it);
	auto classHash = this->getItemClassHash(tmpl->itemClass, tmpl->itemSubClass);
	m_subclassItemLookupTable.emplace(classHash, it);
}

int32 SortedItemBox::getClassItemCount(uint32 itemClass) const
{
	auto count = (int32)m_classItemLookupTable.count(itemClass);
	return count;
}

int32 SortedItemBox::getSubclassItemCount(uint32 itemClass, uint32 itemSubClass) const
{
	auto classHash = this->getItemClassHash(itemClass, itemSubClass);
	auto count = (int32)m_subclassItemLookupTable.count(classHash);
	return count;
}

std::size_t SortedItemBox::getItemClassHash(uint32 itemClass, uint32 itemSubClasss) const
{
	std::size_t seed = 0;
	boost::hash_combine(seed, itemClass);
	boost::hash_combine(seed, itemSubClasss);
	return seed;
}

int32 assignItemsToItemBoxs(std::vector<LootItem>& itemList, std::vector<SortedItemBox>& itemBoxList)
{
	int32 itemCount = 0;
	std::sort(itemList.begin(), itemList.end(), LootItemOrderPred());
	for (auto itemIt = itemList.begin(); itemIt != itemList.end();)
	{
		auto const& lootItem = *itemIt;
		std::sort(itemBoxList.begin(), itemBoxList.end(), ItemBoxOrderPred(lootItem));

		auto it = itemBoxList.begin();
		NS_ASSERT(it != itemBoxList.end());
		auto& itemBox = *it;
		itemBox.addItem(lootItem);

		itemIt = itemList.erase(itemIt);
		++itemCount;
	}

	return itemCount;
}