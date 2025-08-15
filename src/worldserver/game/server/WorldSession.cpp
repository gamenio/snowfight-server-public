#include "WorldSession.h"

#include "protocol/pb/TimeSyncReq.pb.h"
#include "protocol/pb/AuthChallenge.pb.h"
#include "protocol/pb/Ping.pb.h"

#include "protocol/OpcodeHandler.h"
#include "game/behaviors/Player.h"
#include "game/theater/Theater.h"
#include "WorldSocket.h"


#define TIME_SYNC_INTERVAL			10000 // Time synchronization interval. Unit: milliseconds

WorldSession::WorldSession(std::shared_ptr<WorldSocket> const& socket):
	m_sessionId(0),
	m_remoteAddress(socket->getRemoteAddress().to_string()),
	m_remotePort(socket->getRemotePort()),
	m_theater(nullptr),
	m_socket(socket),
	m_isInQueue(false),
	m_isLoggingOut(false),
	m_isLogoutAfterDisconnected(false),
	m_isAuthed(false),
	m_player(nullptr),
	m_gmLevel(0),
	m_requiredCapabilities(0),
	m_latencies(),
	m_totalLatency(0),
	m_latencyCounter(0),
	m_timeSyncClient(0),
	m_timeSyncServer(0),
	m_timeSyncCounter(0),
	m_timeoutTimer(0),
	m_timeoutTime(SESSION_TIMEOUT_NEVER)
{
}

WorldSession::~WorldSession()
{
	if (m_socket)
	{
		m_socket->closeSocket();
		m_socket = nullptr;
	}

	m_theater = nullptr;
}

void WorldSession::addToRecvQueue(WorldPacket&& newPacket)
{
	m_recvQueue.add(std::move(newPacket));
}

void WorldSession::packAndSend(WorldPacket&& packet, MessageLite const& message)
{
	if (!m_socket || !m_socket->isOpen())
		return;

	try
	{
		packet.pack(message);
		m_socket->queuePacket(std::move(packet));
	}
	catch (PacketException const& ex)
	{
		NS_LOG_ERROR("world.session", "Sending to client %s:%d failed because PacketException occurred while packing a packet (opcode: %u), sessionid=%u. errmsg: %s",
			m_remoteAddress.c_str(), m_remotePort, packet.getOpcode(), this->getSessionId(), ex.what());
	}																			
}

uint32 WorldSession::getTheaterId() const
{
	if (m_theater)
		return m_theater->getTheaterId(); 
	else 
		return 0;
}

void WorldSession::kickPlayer()
{
	if (m_isLoggingOut)
		return;

	if (m_socket)
		m_socket->closeSocket();

	m_isLoggingOut = true;
}

void WorldSession::logoutPlayer()
{
	if (!m_player)
		return;

	this->updateAvgLatency();

	NS_LOG_INFO("world.session", "Player(%s) status, %s", m_player->getData()->getGuid().toString().c_str(), m_player->getStatusInfo().c_str());
	NS_LOG_INFO("world.session", "Player(%s) logged out from client %s:%d", m_player->getData()->getGuid().toString().c_str(), m_remoteAddress.c_str(), m_remotePort);
	m_player->cleanupBeforeDelete();
	if (m_theater)
	{
		m_theater->getSpawnManager()->removePlayerFromQueue(m_player);
		bool removed = m_theater->removeSession(this->getSessionId());
		NS_ASSERT(removed);
	}

	if (BattleMap* map = m_player->getMap())
		map->removePlayerFromMap(m_player, true);

	this->setPlayer(nullptr);
}

bool WorldSession::update(NSTime diff)
{
	this->updateTimeoutTimer(diff);
	if (isSessionTimedOut())
	{
		char const* status = "";
		if (m_isAuthed)
			status = "authed";
		else if (m_isInQueue)
			status = "queued";
		else
			status = "expired";
		NS_LOG_WARN("world.session", "Session timed out from client %s:%d, sessionid=%u, status=%s", m_remoteAddress.c_str(), m_remotePort, this->getSessionId(), status);

		this->kickPlayer();
	}

	WorldPacket packet;
	while (m_recvQueue.next(packet))
	{
		try
		{
			// Call the handler corresponding to the opcode to process the protocol data
			if (gOpcodeHandlerTable.find(packet.getOpcode()) != gOpcodeHandlerTable.end())
			{
				OpcodeHandler& opHandler = gOpcodeHandlerTable[packet.getOpcode()];
				switch (opHandler.status)
				{
				case STATUS_AUTHED:
					if (m_isAuthed)
						(this->*opHandler.handler)(packet);
					break;
				case STATUS_LOGGEDIN:
					if (m_player)
						(this->*opHandler.handler)(packet);
					break;
				default:
					break;
				}
			}
			else
				NS_LOG_ERROR("world.session", "Received unhandled opcode %s from %s:%d"
					, getOpcodeNameForLogging(packet.getOpcode()).c_str(), m_remoteAddress.c_str(), m_remotePort);
		}
		catch (PacketException const&)
		{
			NS_LOG_ERROR("world.session", "WorldSession::Update PacketException occurred while unpacking a packet (opcode: %u) from client %s:%d, sessionid=%u. Skipped packet.",
				packet.getOpcode(), m_remoteAddress.c_str(), m_remotePort, this->getSessionId());
		}
	}
	

	if (!m_socket || !m_socket->isOpen())
	{
		if (!m_player || m_isLoggingOut || m_isLogoutAfterDisconnected)
		{
			m_socket = nullptr;
			this->logoutPlayer();

			return false;
		}
	}
	else
	{
		if (m_player)
		{
			// Synchronize time regularly
			m_timeSyncTimer.update(diff);
			if (m_timeSyncTimer.passed())
			{
				this->sendTimeSync();
				m_timeSyncTimer.reset();
			}
		}
	}

	return true;
}

void WorldSession::sendAuthVerdict(AuthVerdict::AuthResult result, int32 waitPos)
{
	AuthVerdict message;
	message.set_result(result);
	message.set_wait_pos(waitPos);
	message.set_gm_level(m_gmLevel);
	message.set_session_id(m_sessionId);

	WorldPacket packet(SMSG_AUTH_VERDICT);
	this->packAndSend(std::move(packet), message);
}

void WorldSession::onSessionAccepted()
{
	m_isInQueue = false;
	m_isAuthed = true;
	m_isLogoutAfterDisconnected = !this->isRequiresCapability(REQUIRES_ALLOW_PLAYER_TO_RESTORE);

	this->resetTimeSync();
	this->sendTimeSync();

	this->sendAuthVerdict(AuthVerdict::AUTH_OK);
}

void WorldSession::onSessionAccepted(WorldSession* oldSession)
{
	std::swap(m_player, oldSession->m_player);
	if (m_player)
		m_player->discardSession();

	m_theater = oldSession->m_theater;
	m_latencyCounter = oldSession->m_latencyCounter;
	m_totalLatency = oldSession->m_totalLatency;
	for (int32 i = 0; i < MAX_LATENCY_INDEXES; ++i)
		m_latencies[i] = oldSession->m_latencies[i];

	this->setPlayerId(oldSession->getPlayerId());
	this->setOriginalPlayerId(oldSession->getOriginalPlayerId());
	this->setGMLevel(oldSession->getGMLevel());
	this->setTimeout(oldSession->getTimeoutTime());
	this->setRequiredCapabilities(oldSession->getRequiredCapabilities());

	this->onSessionAccepted();
}

void WorldSession::onSessionExpired()
{
	this->sendAuthVerdict(AuthVerdict::AUTH_SESSION_EXPIRED);
}

void WorldSession::onSessionQueued(int32 position)
{
	m_isInQueue = true;
	this->sendAuthVerdict(AuthVerdict::AUTH_WAIT_QUEUE, position);
}

NSTime WorldSession::getLatency(LatencyIndex index)
{
	std::lock_guard<std::mutex> lock(m_latencyMutex);

	return m_latencies[index];
}

void WorldSession::setLatency(NSTime latency)
{
	std::lock_guard<std::mutex> lock(m_latencyMutex);

	m_latencies[LATENCY_LATEST] = latency;
	if (m_latencies[LATENCY_MIN] > 0)
		m_latencies[LATENCY_MIN] = std::min(latency, m_latencies[LATENCY_MIN]);
	else
		m_latencies[LATENCY_MIN] = latency;
	m_latencies[LATENCY_MAX] = std::max(latency, m_latencies[LATENCY_MAX]);

	m_totalLatency += latency;
	++m_latencyCounter;
}

void WorldSession::setTimeout(NSTime time)
{
	m_timeoutTime = time;
	this->resetTimeoutTimer();
}

void WorldSession::resetTimeoutTimer()
{
	m_timeoutTimer = m_timeoutTime;
}

void WorldSession::updateTimeoutTimer(NSTime diff)
{
	if (m_timeoutTimer <= 0)
		return;

	m_timeoutTimer -= diff;
}

NSTime WorldSession::getClientNowTimeMillis()
{
	std::lock_guard<std::mutex> lock(m_timeSyncMutex);

	return static_cast<NSTime>(getUptimeMillis() - m_timeSyncClient);
}

void WorldSession::sendTimeSync()
{
	TimeSyncReq timeSync;

	std::unique_lock<std::mutex> lock(m_timeSyncMutex);

	m_timeSyncServer = getUptimeMillis();
	timeSync.set_counter(++m_timeSyncCounter);

	lock.unlock();

	WorldPacket packet(SMSG_TIME_SYNC_REQ);
	this->packAndSend(std::move(packet), timeSync);
}


void WorldSession::resetTimeSync()
{
	std::lock_guard<std::mutex> lock(m_timeSyncMutex);

	m_timeSyncTimer.setInterval(TIME_SYNC_INTERVAL);
	m_timeSyncClient = 0;
	m_timeSyncServer = 0;
	m_timeSyncCounter = 0;
}

void WorldSession::updateClientTime(int32 counter, int32 time)
{
	std::lock_guard<std::mutex> lock(m_timeSyncMutex);

	if (counter != m_timeSyncCounter)
	{
		NS_LOG_WARN("world.session", "Wrong time sync counter from client %s:%d", m_remoteAddress.c_str(), m_remotePort);
		return;
	}

	int64 currTime = getUptimeMillis();

	int32 latency = (int32)(currTime - m_timeSyncServer);
	int32 clientTime = time + latency / 2;

	m_timeSyncClient = currTime - clientTime;

	// NS_LOG_DEBUG("world.session", "TIME SYNC clienttime: %dms latency: %dms counter: %d client: %s:%d", clientTime, latency, counter, m_remoteAddress.c_str(), m_remotePort);
}

void WorldSession::sendFlashMessage(FlashMessage const& message)
{
	WorldPacket packet(SMSG_FLASH_MESSAGE);
	this->packAndSend(std::move(packet), message);
}

void WorldSession::sendPlayerActionMessage(PlayerActionMessage const& message)
{
	WorldPacket packet(SMSG_PLAYER_ACTION_MESSAGE);
	this->packAndSend(std::move(packet), message);
}

void WorldSession::updateAvgLatency()
{
	std::lock_guard<std::mutex> lock(m_latencyMutex);

	if (m_latencyCounter <= 0)
		return;

	NSTime avg = (NSTime)(m_totalLatency / m_latencyCounter);
	if (m_latencies[LATENCY_AVG] > 0)
		m_latencies[LATENCY_AVG] = (m_latencies[LATENCY_AVG] + avg) / 2;
	else
		m_latencies[LATENCY_AVG] = avg;

	m_latencyCounter = 0;
	m_totalLatency = 0;
}
