#include "ObjectSearcher.h"

#include "game/behaviors/WorldObject.h"
#include "game/behaviors/Player.h"
#include "game/behaviors/Robot.h"

bool PlayerRangeExistsObjectFilter::operator()(Player* target)
{
	if (target == m_object)
		return false;

	if (!target->hasAtClient(m_object))
		return false;

	return target->canSeeOrDetect(m_object);
}

bool PlayerRangeExistsLocatorFilter::operator()(Player* target)
{
	if (target == m_object->getOwner())
		return false;

	if (!target->hasTrackingAtClient(m_object->getOwner()))
		return false;

	return target->canTrack(m_object->getOwner());
}

bool PlayerClientExistsObjectFilter::operator()(Player* target)
{
	if (target == m_object)
		return false;

	if (target->hasAtClient(m_object))
	{
		return true;
	}
	return false;
}

bool PlayerClientExistsLocatorFilter::operator()(Player* target)
{
	if (target == m_object->getOwner())
		return false;

	if (target->hasTrackingAtClient(m_object->getOwner()))
	{
		return true;
	}
	return false;
}

bool PlayerRangeContainsOneOfObjectsFilter::operator()(Player* target)
{
	if (m_exclusions.find(target) != m_exclusions.end())
		return false;

	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		if (!target->hasAtClient(*it))
			continue;

		if (target->canSeeOrDetect(*it))
			return true;
	}

	return false;
}

void ValuesUpdateAccumulator::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();
		this->buildUpdate(player);
	}
}


void ValuesUpdateAccumulator::buildUpdate(Player* player)
{
	if (!player->hasAtClient(&m_object))
		return;

	if (player != &m_object && !player->canSeeOrDetect(&m_object))
		return;

	auto iter = m_updateMap.find(player);
	if (iter == m_updateMap.end())
	{
		std::pair<PlayerUpdateMapType::iterator, bool> p = m_updateMap.emplace(player, UpdateObject());
		NS_ASSERT(p.second);
		iter = p.first;
	}

	m_object.buildValuesUpdateBlockForPlayer(iter->first, &iter->second);
}

void LocatorValuesUpdateAccumulator::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();
		this->buildUpdate(player);
	}
}

void LocatorValuesUpdateAccumulator::buildUpdate(Player* player)
{
	if (!player->hasTrackingAtClient(m_object.getOwner()))
		return;

	if (player == m_object.getOwner() || !player->canTrack(m_object.getOwner()))
		return;

	auto iter = m_updateMap.find(player);
	if (iter == m_updateMap.end())
	{
		auto p = m_updateMap.emplace(player, UpdateObject());
		NS_ASSERT(p.second);
		iter = p.first;
	}

	m_object.buildValuesUpdateBlockForPlayer(iter->first, &iter->second);
}