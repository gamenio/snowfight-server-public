#include "GridNotifiers.h"

#include "game/dynamic/TypeContainerVisitor.h"
#include "GridAreaImpl.h"

void TrackingNotifier::sendToSelf()
{
	if (!m_updateObject.hasUpdate())
		return;

	if (WorldSession* session = m_player.getSession())
	{
		WorldPacket packet(SMSG_UPDATE_OBJECT);
		session->packAndSend(std::move(packet), m_updateObject);
	}
}

void TrackingChangesNotifier::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();
		if (player == &m_object)
			continue;

		player->updateTraceabilityOf(&m_object);
	}
}

void PlayerTrackingNotifier::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();

		m_player.updateTraceabilityOf(player, m_updateObject);

		if (!player->isNeedNotify(NOTIFY_TRACEABILITY_CHANGED))
			player->updateTraceabilityOf(&m_player);
	}
}

void PlayerTrackingNotifier::visit(RobotGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Robot* robot = it->getSource();

		m_player.updateTraceabilityOf(robot, m_updateObject);
	}
}

void RobotTrackingNotifier::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();

		if (!player->isNeedNotify(NOTIFY_TRACEABILITY_CHANGED))
			player->updateTraceabilityOf(&m_robot);
	}
}

void VisibleNotifier::sendToSelf()
{
	if (!m_updateObject.hasUpdate())
		return;

	if (WorldSession* session = m_player.getSession())
	{
		WorldPacket packet(SMSG_UPDATE_OBJECT);
		session->packAndSend(std::move(packet), m_updateObject);
	}

	for (auto it = m_visibleNow.begin(); it != m_visibleNow.end(); ++it)
		(*it)->sendInitialVisiblePacketsToPlayer(&m_player);
}

inline void robotUnitRelocationWorker(Robot* robot, Unit* unit)
{
	if (robot->canSeeOrDetect(unit))
	{
		if(robot->getAI())
			robot->getAI()->moveInLineOfSight(unit);
	}
}

inline void robotItemBoxRelocationWorker(Robot* robot, ItemBox* itemBox)
{
	if (robot->canSeeOrDetect(itemBox))
	{
		if(robot->getAI())
			robot->getAI()->moveInLineOfSight(itemBox);
	}
}

inline void robotItemRelocationWorker(Robot* robot, Item* item)
{
	if (item->canDetect(robot))
	{
		item->testCollision(robot);
	}

	if (robot->canSeeOrDetect(item))
	{
		if(robot->getAI())
			robot->getAI()->moveInLineOfSight(item);
	}
}

inline void robotProjectileRelocationWorker(Robot* robot, Projectile* proj)
{
	if (robot->canSeeOrDetect(proj))
	{
		if(robot->getAI())
			robot->getAI()->moveInLineOfSight(proj);
	}
}

inline void playerItemBoxRelocationWorker(Player* player, ItemBox* itemBox)
{
	if (player->canSeeOrDetect(itemBox))
	{
		player->getAI()->moveInLineOfSight(itemBox);
	}
}

inline void playerItemRelocationWorker(Player* player, Item* item)
{
	if (item->canDetect(player))
	{
		item->testCollision(player);
	}

	if (player->canSeeOrDetect(item))
	{
		player->getAI()->moveInLineOfSight(item);
	}
}

inline void projectileUnitRelocationWorker(Projectile* proj, Unit* unit)
{
	if (proj->canDetect(unit))
	{
		proj->testCollision(unit);
	}
}

inline void projectileItemBoxRelocationWorker(Projectile* proj, ItemBox* itemBox)
{
	if (proj->canDetect(itemBox))
	{
		proj->testCollision(itemBox);
	}
}

void VisibleChangesNotifier::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();
		if(player == &m_object)
			continue;

		player->updateVisibilityOf(&m_object);
	}
}


void RobotRelocationNotifier::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();

		if (!player->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
			player->updateVisibilityOf(&m_robot);

		robotUnitRelocationWorker(&m_robot, player);
	}
}

void RobotRelocationNotifier::visit(RobotGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Robot* robot = it->getSource();

		robotUnitRelocationWorker(&m_robot, robot);

		if(!robot->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
			robotUnitRelocationWorker(robot, &m_robot);
	}
}

void RobotRelocationNotifier::visit(ItemBoxGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		ItemBox* itemBox = it->getSource();
		robotItemBoxRelocationWorker(&m_robot, itemBox);
	}
}

void RobotRelocationNotifier::visit(ItemGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Item* item = it->getSource();
		robotItemRelocationWorker(&m_robot, item);
	}
}

void RobotRelocationNotifier::visit(ProjectileGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Projectile* proj = it->getSource();

		if (!proj->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
			projectileUnitRelocationWorker(proj, &m_robot);

		robotProjectileRelocationWorker(&m_robot, proj);
	}
}

void PlayerRelocationNotifier::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();

		m_player.updateVisibilityOf(player, m_updateObject, m_visibleNow);

		// m_player有可能从player的可视范围内出现或移出，如果出现则需要将m_player的可见状态发送给player
		if (!player->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
		{
			player->updateVisibilityOf(&m_player);
		}
	}
}

void PlayerRelocationNotifier::visit(RobotGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Robot* robot = it->getSource();

		m_player.updateVisibilityOf(robot, m_updateObject, m_visibleNow);

		if (!robot->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
			robotUnitRelocationWorker(robot, &m_player);
	}
}

void PlayerRelocationNotifier::visit(ItemBoxGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		ItemBox* itemBox = it->getSource();

		if (!itemBox->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
			playerItemBoxRelocationWorker(&m_player, itemBox);

		m_player.updateVisibilityOf(itemBox, m_updateObject, m_visibleNow);
	}
}

void PlayerRelocationNotifier::visit(ItemGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Item* item = it->getSource();

		if (!item->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
			playerItemRelocationWorker(&m_player, item);

		m_player.updateVisibilityOf(item, m_updateObject, m_visibleNow);
	}
}

void PlayerRelocationNotifier::visit(ProjectileGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Projectile* proj = it->getSource();

		m_player.updateVisibilityOf(proj, m_updateObject, m_visibleNow);

		if (!proj->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
			projectileUnitRelocationWorker(proj, &m_player);
	}
}

void ItemBoxRelocationNotifier::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();

		playerItemBoxRelocationWorker(player, &m_itemBox);

		if (!player->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
			player->updateVisibilityOf(&m_itemBox);

	}
}

void ItemBoxRelocationNotifier::visit(RobotGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Robot* robot = it->getSource();
		robotItemBoxRelocationWorker(robot, &m_itemBox);
	}
}

void ItemRelocationNotifier::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();

		if (!player->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
			player->updateVisibilityOf(&m_item);

		playerItemRelocationWorker(player, &m_item);
	}
}

void ItemRelocationNotifier::visit(RobotGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Robot* robot = it->getSource();
		robotItemRelocationWorker(robot, &m_item);
	}
}

void ProjectileRelocationNotifier::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();

		if (!player->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
			player->updateVisibilityOf(&m_proj);

		projectileUnitRelocationWorker(&m_proj, player);
	}
}

void ProjectileRelocationNotifier::visit(RobotGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Robot* robot = it->getSource();

		if (!robot->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
			robotProjectileRelocationWorker(robot, &m_proj);

		projectileUnitRelocationWorker(&m_proj, robot);
	}
}

void ProjectileRelocationNotifier::visit(ItemBoxGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		ItemBox* itemBox = it->getSource();
		projectileItemBoxRelocationWorker(&m_proj, itemBox);
	}
}

void ObjectRelocation::visit(RobotGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Robot* robot = it->getSource();

		if (robot->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
		{
			RobotRelocationNotifier notifier(*robot);
			m_map.visitGridsInMaxVisibleRange(robot->getData()->getPosition(), notifier, true);
			robot->updateDangerState();
		}
		if (robot->isNeedNotify(NOTIFY_SAFETY_CHANGED))
		{
			robot->updateDangerState();
		}
		if (robot->isNeedNotify(NOTIFY_TRACEABILITY_CHANGED))
		{
			RobotTrackingNotifier notifier(*robot);
			m_map.visitCreatedGrids(notifier);
		}
	}
}

void ObjectRelocation::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();

		if (player->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
		{
			PlayerRelocationNotifier notifier(*player);
			m_map.visitGridsInMaxVisibleRange(player->getData()->getPosition(), notifier, true);
			notifier.sendToSelf();
		}
		if (player->isNeedNotify(NOTIFY_SAFETY_CHANGED))
		{
			player->updateDangerState();
		}
		if (player->isNeedNotify(NOTIFY_TRACEABILITY_CHANGED))
		{
			PlayerTrackingNotifier notifier(*player);
			m_map.visitCreatedGrids(notifier);
			notifier.sendToSelf();
		}
	}
}

void ObjectRelocation::visit(ItemBoxGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		ItemBox* itemBox = it->getSource();

		if (itemBox->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
		{
			ItemBoxRelocationNotifier notifier(*itemBox);
			m_map.visitGridsInMaxVisibleRange(itemBox->getData()->getPosition(), notifier);
		}
	}
}

void ObjectRelocation::visit(ItemGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Item* item = it->getSource();

		if (item->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
		{
			ItemRelocationNotifier notifier(*item);
			m_map.visitGridsInMaxVisibleRange(item->getData()->getPosition(), notifier);
		}
	}
}

void ObjectRelocation::visit(ProjectileGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Projectile* proj = it->getSource();

		if (proj->isNeedNotify(NOTIFY_VISIBILITY_CHANGED))
		{
			ProjectileRelocationNotifier notifier(*proj);
			m_map.visitGridsInMaxVisibleRange(proj->getData()->getPosition(), notifier);
		}
	}
}

void SafeZoneRelocationNotifier::visit(PlayerGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Player* player = it->getSource();
		if (!player->isNeedNotify(NOTIFY_SAFETY_CHANGED))
			player->updateDangerState();
	}
}

void SafeZoneRelocationNotifier::visit(RobotGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Robot* robot = it->getSource();
		if (!robot->isNeedNotify(NOTIFY_SAFETY_CHANGED))
			robot->updateDangerState();
	}
}

void ObjectUpdateNotifier::visit(RobotGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
	{
		Robot* robot = it->getSource();
		robot->update(m_diff);

		// 更新周围活动状态的Grid中的被动对象，例如：Item
		GridArea area = computeGridAreaInCircle(robot->getData()->getPosition(), robot->getSightDistance());
		m_map.updateObjectsInGridArea(area, m_updaterVisitor);
	}
}

void ObjectUpdateNotifier::visit(ProjectileGridType& g)
{
	for (auto it = g.begin(); it != g.end(); ++it)
		it->getSource()->update(m_diff);
}
