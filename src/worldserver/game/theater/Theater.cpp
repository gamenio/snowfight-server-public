#include "Theater.h"

#include "game/server/protocol/pb/WaitForPlayers.pb.h"

#include "utilities/Random.h"
#include "utilities/TimeUtil.h"
#include "game/maps/MapDataManager.h"
#include "game/behaviors/Player.h"
#include "game/world/ObjectMgr.h"

#define MAX_PLAYER_NAME_ID_COUNTER					0x0FFF

Theater::Theater(uint32 theaterId) :
	m_theaterId(theaterId),
	m_playerCount(0),
	m_battleCount(0),
	m_gmCount(0),
	m_state(STATE_IDLE),
	m_map(nullptr),
	m_spawnManager(new SpawnManager()),
	m_updateDiff(0),
	m_isDeleting(false),
	m_deletionDelay(0),
	m_deletionStartTime(0)
{
	m_spawnManager->setTheater(this);
}

Theater::~Theater()
{
	if (m_map)
	{
		delete m_map;
		m_map = nullptr;
	}

	if (m_spawnManager)
	{
		delete m_spawnManager;
		m_spawnManager = nullptr;
	}
}

void Theater::addSession(WorldSession* session)
{
	auto ret = m_sessions.emplace(session->getSessionId(), session);

	if (this->isDeleting())
		this->undelete();

	bool isMapCreated = this->createMapIfNotExistForPlayer(session);
	if (isMapCreated || this->isSleepState())
		this->configureMapForPlayer(session);

	if (ret.second)
	{
		if (session->hasGMPermission())
			++m_gmCount;
		else
			++m_playerCount;
	}
}

bool Theater::acceptSession(WorldSession* session)
{
	NS_ASSERT(m_map, "The map is not created.");
	Player* player = session->getPlayer();
	NS_ASSERT(player, "The player is not logged in.");

	if (this->getOnlineCount() <= 0 && m_map->getMapId() != player->getData()->getSelectedMapId())
		return false;

	// 对战人数达到上限
	int32 combatants = this->getOnlineCount() + m_map->getRobotCount();
	if (combatants >= this->getPopulationCap())
		return false;

	if (!m_map->canJoinBattle())
		return false;

	// 战区不在睡眠状态
	if (!this->isSleepState())
	{
		if (player->getData()->isTrainee() 
			|| m_map->isTrainingGround()
			|| !m_map->isWithinCombatGrade(player->getData()->getCombatPower()))
		{
			return false;
		}
	}

	this->addSession(session);

	return true;
}

bool Theater::replaceSession(WorldSession* session)
{
	auto it = m_sessions.find(session->getSessionId());
	if (it != m_sessions.end())
	{
		(*it).second = session;
		return true;
	}

	return false;
}

bool Theater::waitForPlayers()
{
	if (m_state == STATE_IDLE)
	{
		m_state = STATE_WAITING_FOR_PLAYERS;
		m_waitForPlayersTimer.reset();
	}

	return m_state == STATE_WAITING_FOR_PLAYERS;
}

void Theater::skipWaitingPlayers()
{
	if (!m_waitForPlayersTimer.passed())
		m_waitForPlayersTimer.setPassed();
}

void Theater::sendWaitForPlayersToPlayer(WorldSession* session)
{
	WorldPacket packet(SMSG_WAIT_FOR_PLAYERS);

	WaitForPlayers message;
	NSTime startTime = session->getClientNowTimeMillis() - m_waitForPlayersTimer.getElapsed();
	message.set_start_time(startTime);
	message.set_duration(m_waitForPlayersTimer.getDuration());

	session->packAndSend(std::move(packet), message);
}

void Theater::setWaitForPlayersTimeout(NSTime time)
{
	m_waitForPlayersTimer.setDuration(time);
}

float Theater::getOrder(WorldSession* session) const
{
	NS_ASSERT(m_map, "The map is not created.");
	Player* player = session->getPlayer();
	NS_ASSERT(player, "The player is not logged in.");

	float weight = 0;

	// 如果有玩家在线或者玩家选定的地图与当前战区地图匹配
	if (this->getOnlineCount() > 0 || m_map->getMapId() == player->getData()->getSelectedMapId())
		weight += 1.0f;

	int32 combatants = this->getOnlineCount() + m_map->getRobotCount();
	// 对战人数没有达到上限
	if (combatants < this->getPopulationCap())
	{
		weight += 1.0f;
		// 在线人数占人口上限比例
		weight += (float)this->getOnlineCount() / this->getPopulationCap();
	}

	// 能够加入战斗
	if (m_map->canJoinBattle())
		weight += 1.0f;

	// 战区处于睡眠状态
	if (this->isSleepState())
		weight += 1.0f;
	else
	{
		if (!player->getData()->isTrainee() // 不是训练场学员
			&& !m_map->isTrainingGround() // 地图不是训练场
			&& m_map->isWithinCombatGrade(player->getData()->getCombatPower())) // 玩家战斗等级在区间范围内
		{
			weight += 1.0f;
		}
	}

	return weight;
}

bool Theater::removeSession(uint32 id)
{
	bool removed = false;
	auto it = m_sessions.find(id);
	if (it != m_sessions.end())
	{
		WorldSession* session = (*it).second;
		m_sessions.erase(it);

		if (session->hasGMPermission())
			--m_gmCount;
		else
			--m_playerCount;

		removed = true;
	}

	if (this->getOnlineCount() <= 0)
		this->deleteDelayed();
		
	return removed;
}

void Theater::advancedUpdate(NSTime diff)
{
	m_updateDiff = diff;

	if (this->getOnlineCount() <= 0)
		return;

	if (m_state == STATE_WAITING_FOR_PLAYERS)
	{
		m_waitForPlayersTimer.update(diff);
		if (m_waitForPlayersTimer.passed())
		{
			if (m_spawnManager->getPlayersInPlaceCount() > 0)
			{
				m_spawnManager->onPlayersInPlace();
				m_state = STATE_PLAYERS_IN_PLACE;
			}
		}
	}
	else
	{
		if (m_state == STATE_ACTIVE)
			m_spawnManager->advancedUpdate(diff);
	}
}

void Theater::update(NSTime diff)
{
	if (!m_map)
		return;

	if (m_state == STATE_IDLE)
		return;

	if (m_state == STATE_PLAYERS_IN_PLACE)
	{
		m_map->onStart();
		m_state = STATE_ACTIVE;
		++m_battleCount;
	}
	else
	{
		if (this->getOnlineCount() <= 0)
		{
			if(m_state == STATE_ACTIVE)
				m_map->onStop();
			m_state = STATE_IDLE;
		}
	}

	if(m_state == STATE_ACTIVE)
		m_map->update(diff);
}

void Theater::setDeletionDelay(NSTime delay)
{
	m_deletionDelay = delay;
	m_deletionStartTime = 0;
}

bool Theater::canDelete() const
{
	if (!m_isDeleting)
		return false;

	int64 diff = getUptimeMillis() - m_deletionStartTime;
	return diff >= m_deletionDelay;
}

void Theater::deleteDelayed()
{
	if (m_isDeleting)
		return;

	m_deletionStartTime = getUptimeMillis();
	m_isDeleting = true;
}

void Theater::undelete()
{
	if (!m_isDeleting)
		return;

	m_deletionStartTime = 0;
	m_isDeleting = false;
}

bool Theater::createMapIfNotExistForPlayer(WorldSession* session)
{
	if (m_map)
		return false;

	Player* player = session->getPlayer();
	NS_ASSERT(player, "The player is not logged in.");
	MapData* mapData = sMapDataManager->findMapData(player->getData()->getSelectedMapId());
	NS_ASSERT(mapData, "No MapData with ID %d found.", player->getData()->getSelectedMapId());
	m_map = new BattleMap(mapData, m_spawnManager);

	return true;
}

void Theater::configureMapForPlayer(WorldSession* session)
{
	NS_ASSERT(m_map, "The map is not created.");

	Player* player = session->getPlayer();
	NS_ASSERT(player, "The player is not logged in.");
	CombatGrade const* combatGrade = sObjectMgr->getCombatGrade(player->getData()->getCombatGrade());
	NS_ASSERT(combatGrade, "No CombatGrade with grade %d found.", player->getData()->getCombatGrade());
	m_map->setCombatGrade(*combatGrade);
	m_map->setPrimaryLang(player->getData()->getLang());
	m_map->prepareForBattle();
}