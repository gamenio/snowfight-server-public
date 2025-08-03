#include "TheaterManager.h"

#include "configuration/Config.h"
#include "game/behaviors/Player.h"
#include "game/maps/MapDataManager.h"

static uint32 sSessionCounter = 0;
static uint32 sTheaterCounter = 0;


TheaterManager::TheaterManager():
	m_isStopping(false),
	m_isStopped(false),
	m_playerLimit(0),
	m_playerCount(0),
	m_gmCount(0),
	m_theaterDeletionDelay(0),
	m_waitForPlayersTimeout(0),
	m_sessionTimeout(0),
	m_expiredSessionDelay(0),
	m_queuedSessionTimeout(0)
{

}


TheaterManager::~TheaterManager()
{
	m_updater.stop();

	WorldSession* sess = nullptr;
	while (m_pendingSessions.next(sess))
		delete sess;

	NS_ASSERT(m_orderedTheaters.empty());
	NS_ASSERT(m_theaters.empty());

	NS_ASSERT(m_queuedPlayers.empty());
	NS_ASSERT(m_sessions.empty());
}

uint32 TheaterManager::generateSessionId()
{
	uint32 magic = random(0, 0xFFFF);
	if (sSessionCounter >= 0xFFFF)
		sSessionCounter = 0;
	uint32 counter = ++sSessionCounter;
	uint32 id = magic << 16 | counter;
	return id;
}

TheaterManager* TheaterManager::instance()
{
	static TheaterManager instance;
	return &instance;
}

void TheaterManager::initialize()
{
	m_playerLimit = sConfigMgr->getIntDefault("PlayerLimit", 1000);
	m_theaterDeletionDelay = sConfigMgr->getIntDefault("TheaterDeletionDelay", 3600);
	m_waitForPlayersTimeout = sConfigMgr->getIntDefault("WaitForPlayersTimeout", 5000);

	int32 updateThreads = sConfigMgr->getIntDefault("TheaterUpdateThreads", 1);
	m_updater.start(updateThreads);

	m_sessionTimeout = sConfigMgr->getIntDefault("SessionTimeout", 60000);
	m_expiredSessionDelay = sConfigMgr->getIntDefault("ExpiredSessionDelay", 5000);
	m_queuedSessionTimeout = sConfigMgr->getIntDefault("QueuedSessionTimeout", 10000);
}

void TheaterManager::stop()
{
	m_isStopping = true;
}

void TheaterManager::kickAll()
{
	for (auto it = m_queuedPlayers.begin(); it != m_queuedPlayers.end(); ++it)
		(*it)->kickPlayer();

	for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it)
		(it->second)->kickPlayer();
}

bool TheaterManager::tryAddSession(WorldSession* session)
{
	if (m_playerCount >= m_playerLimit)
		return false;

	session->setSessionId(this->generateSessionId());

	m_sessions[session->getSessionId()] = session;
	++m_playerCount;

	session->onSessionAccepted();
	session->setTimeout(m_sessionTimeout);

	return true;
}

bool TheaterManager::tryRestoreSession(WorldSession* session)
{
	auto it = m_sessions.find(session->getSessionId());
	if (it == m_sessions.end())
		return false;

	WorldSession* oldSession = (*it).second;
	// 未进入世界的玩家不能恢复会话
	if (!oldSession->getPlayer() || !oldSession->getPlayer()->isInWorld())
		return false;

	Theater* theater = oldSession->getTheater();
	NS_ASSERT(theater != nullptr, "Cannot restore from old session because Theater is not specified.");

	bool replaced = theater->replaceSession(session);
	NS_ASSERT(replaced, "Failed to replace old session in Theater.");

	m_sessions[oldSession->getSessionId()] = session;
	session->onSessionAccepted(oldSession);

	// 将旧会话的网络连接断开并释放，所有未处理的消息将会忽略
	oldSession->kickPlayer();
	delete oldSession;

	return true;
}

void TheaterManager::addGMSession(WorldSession* session)
{
	session->setSessionId(this->generateSessionId());
	m_sessions[session->getSessionId()] = session;
	++m_gmCount;

	session->onSessionAccepted();
	if(session->isRequiresCapability(REQUIRES_ALLOW_PLAYER_TO_RESTORE))
		session->setTimeout(m_sessionTimeout);
	else
		session->setTimeout(WorldSession::SESSION_TIMEOUT_NEVER);
}

void TheaterManager::updateSessions(NSTime diff)
{
	for (auto it = m_sessions.begin(); it != m_sessions.end();)
	{
		WorldSession* session = (*it).second;
		if (!session->update(diff))
		{
			if (session->hasGMPermission())
				--m_gmCount;
			else
				--m_playerCount;

			it = m_sessions.erase(it);
			delete session;
		}
		else
		{
			++it;
		}
	}
}

void TheaterManager::queueSession(WorldSession* session)
{
	m_pendingSessions.add(session);
}

WorldSession const* TheaterManager::findSession(uint32 id) const
{
	auto it = m_sessions.find(id);
	if (it != m_sessions.end())
		return (*it).second;

	return nullptr;
}


Theater* TheaterManager::createTheater()
{
	Theater* theater = new Theater(++sTheaterCounter);
	NSTime delDelayInMillis = (NSTime)(m_theaterDeletionDelay * 1000);
	theater->setDeletionDelay(delDelayInMillis);
	theater->setWaitForPlayersTimeout(m_waitForPlayersTimeout);

	m_theaters[theater->getTheaterId()] = theater;
	m_orderedTheaters.push_back(theater);

	return theater;
}

Theater const* TheaterManager::findTheater(uint32 id) const
{
	auto it = m_theaters.find(id);
	if (it != m_theaters.end())
		return (*it).second;

	return nullptr;
}


Theater* TheaterManager::selectTheaterForPlayer(WorldSession* session)
{
	if (m_orderedTheaters.empty())
		return nullptr;

	if (m_orderedTheaters.size() > 1)
	{
		std::sort(m_orderedTheaters.begin(), m_orderedTheaters.end(), [session](Theater const* a, Theater const* b) {
			return a->getOrder(session) > b->getOrder(session);
		});
	}

	return m_orderedTheaters.front();
}

void TheaterManager::addPlayerToTheater(WorldSession* session)
{
	Theater* theater = this->selectTheaterForPlayer(session);
	// 如果没有战区或者战区不能接受会话则创建新的战区
	if (!theater || !theater->acceptSession(session))
	{
		theater = this->createTheater();
		theater->addSession(session);
	}
	session->onSessionAcceptedByTheater(theater);
}

uint16 TheaterManager::selectMapForPlayer(WorldSession* session) const
{
	uint16 mapId;
	Player* player = session->getPlayer();
	NS_ASSERT(player, "The player is not logged in.");

	if (player->getData()->isTrainee())
		mapId = TRAINING_GROUND_MAP_ID;
	else
	{
		auto* mapGradeList = sMapDataManager->getMapGradeList(player->getData()->getCombatGrade());
		NS_ASSERT(mapGradeList);
		auto* weightList = sMapDataManager->getMapGradeWeightList(player->getData()->getCombatGrade());
		NS_ASSERT(weightList);

		// 随机选择一个地图
		auto ranIt = randomWeightedContainerElement(*mapGradeList, *weightList);
		NS_ASSERT(ranIt != mapGradeList->end());
		MapGrade const& mapGrade = *ranIt;
		mapId = mapGrade.mapId;
	}

	return mapId;
}

bool TheaterManager::update(NSTime diff)
{
	if (m_isStopped)
		return false;

	if (m_isStopping)
	{
		this->kickAll();
		m_isStopping = false;
		m_isStopped = true;
	}

	// 处理未决的会话。如果服务已经停止则未处理的Session将在析构函数中被释放
	if(!m_isStopped)
		this->processPendingSessions();

	// 更新会话
	this->updateSessions(diff);

	// 更新等待中的玩家
	this->updateQueuedPlayers(diff);

	// 更新过期的玩家
	this->updateExpiredPlayers(diff);

	// 更新战区
	this->updateTheaters(diff);

	// 清除战区
	this->purgeTheaters(m_isStopped);

	return !m_isStopped;
}


void TheaterManager::purgeTheaters(bool force)
{
	for (auto iter = m_orderedTheaters.begin(); iter != m_orderedTheaters.end();)
	{
		Theater* theater = *iter;
		if (theater->canDelete() || force)
		{
			auto it = m_theaters.find(theater->getTheaterId());
			if (it != m_theaters.end())
				m_theaters.erase(it);

			iter = m_orderedTheaters.erase(iter);

			delete theater;
		}
		else
			++iter;	
	}
}

void TheaterManager::updateTheaters(NSTime diff)
{
	for (Theater* theater : m_orderedTheaters)
	{
		theater->advancedUpdate(diff);
	}

	for (Theater* theater : m_orderedTheaters)
	{
		m_updater.scheduleUpdate(diff, theater);
	}
	m_updater.waitUpdate();
}

void TheaterManager::processPendingSessions()
{
	// 尝试将未决列表中的会话恢复或者加入到战区
	WorldSession* newSession = nullptr;
	while (m_pendingSessions.next(newSession))
	{
		if (newSession->getSessionId() > 0)
		{
			if (!this->tryRestoreSession(newSession))
				this->addExpiredPlayer(newSession);
		} 
		else 
		{
			if (newSession->hasGMPermission())
				this->addGMSession(newSession);
			else
			{
				if (!this->tryAddSession(newSession))
					this->addQueuedPlayer(newSession);
			}
		}
	}
}

void TheaterManager::updateQueuedPlayers(NSTime diff)
{
	int32 position = 1;
	bool reposition = false;
	for (auto it = m_queuedPlayers.begin(); it != m_queuedPlayers.end(); )
	{
		WorldSession* session = *it;
		if (!session->update(diff))
		{
			it = m_queuedPlayers.erase(it);
			delete session;
			reposition = true;
		}
		else if (this->tryAddSession(session))
		{
			it = m_queuedPlayers.erase(it);
			reposition = true;
		}
		else
		{
			if (reposition)
				NS_LOG_DEBUG("world.theater", "Update queued player to position %d, address %s:%d.", position, session->getRemoteAddress().c_str(), session->getRemotePort());
			++it;
			++position;
		}
	}
}

int32 TheaterManager::getQueuedPosition(WorldSession* session)
{
	int32 pos = 1;
	for (auto it = m_queuedPlayers.begin(); it != m_queuedPlayers.end(); ++it, ++pos)
		if (*it == session)
			return pos;

	return 0;
}

void TheaterManager::addQueuedPlayer(WorldSession* session)
{
	m_queuedPlayers.push_back(session);
	int32 position = this->getQueuedPosition(session);
	session->onSessionQueued(position);
	session->setTimeout(m_queuedSessionTimeout);
	NS_LOG_DEBUG("world.theater", "Add queued player to position %d, address: %s:%d.", position, session->getRemoteAddress().c_str(), session->getRemotePort());
}

void TheaterManager::updateExpiredPlayers(NSTime diff)
{
	for (auto it = m_expiredPlayers.begin(); it != m_expiredPlayers.end(); )
	{
		WorldSession* session = *it;
		if (!session->update(diff))
		{
			it = m_expiredPlayers.erase(it);
			delete session;
		}
		else
			++it;
	}
}

void TheaterManager::addExpiredPlayer(WorldSession* session)
{
	m_expiredPlayers.push_back(session);
	session->onSessionExpired();
	session->setTimeout(m_expiredSessionDelay);
}