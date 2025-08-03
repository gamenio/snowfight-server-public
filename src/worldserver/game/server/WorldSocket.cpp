#include "WorldSocket.h"

#include "protocol/pb/AuthChallenge.pb.h"
#include "protocol/pb/Ping.pb.h"
#include "protocol/pb/Pong.pb.h"
#include "protocol/pb/TimeSyncResp.pb.h"
#include "protocol/pb/Ping.pb.h"
#include "protocol/pb/AuthProof.pb.h"

#include "utilities/TimeUtil.h"
#include "protocol/Opcode.h"
#include "game/theater/TheaterManager.h"
#include "WorldSession.h"

WorldSocket::WorldSocket(tcp::socket&& socket) :
	Socket(std::move(socket)),
	m_session(nullptr)
{
}

WorldSocket::~WorldSocket()
{
}

void WorldSocket::start()
{
	this->checkIP();
}


void WorldSocket::onReceivedData(WorldPacket&& packet)
{
	std::unique_lock<std::mutex> sessLock(m_sessMutex, std::defer_lock);
	try
	{
		switch (packet.getOpcode())
		{
		case MSG_PING:
			this->handlePing(packet);
			break;
		case CMSG_TIME_SYNC_RESP:
			this->handleTimeSyncResp(packet);
			break;
		case CMSG_AUTH_PROOF:
			this->handleAuthProof(packet);
			break;
		default:
			sessLock.lock();
			if (m_session)
			{
				m_session->addToRecvQueue(std::move(packet));
			}

			break;
		}
	}
	catch (PacketException const&)
	{
		NS_LOG_ERROR("world.socket", "WorldSocket::onReceivedData PacketException occurred while unpacking a packet (opcode: %u) from client %s:%d",
			packet.getOpcode(), this->getRemoteAddress().to_string().c_str(), this->getRemotePort());
	}
}

// 当onSocketClosed()被调用时WorldSession对象可能已经被释放，所以在这里不应该对m_session做任何引用
void WorldSocket::onSocketClosed()
{
	std::lock_guard<std::mutex> lock(m_sessMutex);
	m_session = nullptr;
}

void WorldSocket::checkIP()
{
	// TODO 如果IP没有在黑名单则发起验证请求
	this->asyncRead();
	//this->sendAuthChallenge();
}

//void WorldSocket::sendAuthChallenge()
//{
//	// TODO 生成一个随机数用于协议数据加密
//	AuthChallenge message;
//	message.set_auth_seed(9999);
//
//	WorldPacket packet(SMSG_AUTH_CHALLENGE);
//	this->packAndSend(std::move(packet), message);
//}


void WorldSocket::handleAuthProof(WorldPacket& packet)
{
	AuthProof message;
	packet.unpack(message);

	std::string proof = message.proof();
	uint32 sessionId = message.session_id();
	uint8 gmLevel = proof == "GM" ? 1 : 0;
	uint32 requiredCapabilities = message.required_capabilities();
	std::string playerId = message.playerid();
	std::string originalPlayerId = message.original_playerid();

	// TODO 登录凭证验证通过后创建会话

	std::lock_guard<std::mutex> lock(m_sessMutex);

	m_session = new WorldSession(this->shared_from_this());
	m_session->setPlayerId(playerId);
	m_session->setOriginalPlayerId(originalPlayerId);

	// 如果设置了SessionID, TheaterManager将尝试从相同ID的会话中恢复
	if(sessionId > 0)
		m_session->setSessionId(sessionId);

	m_session->setGMLevel(gmLevel);
	if (m_session->hasGMPermission())
		this->setSendQueueLimit(SEND_QUEUE_UNLIMITED);

	m_session->setRequiredCapabilities(requiredCapabilities);

	sTheaterManager->queueSession(m_session);
}

void WorldSocket::handlePing(WorldPacket& packet)
{
	Ping ping;
	packet.unpack(ping);

	std::unique_lock<std::mutex> lock(m_sessMutex);
	if (!m_session)
	{
		NS_LOG_WARN("world.socket", "WorldSocket::handlePing: peer sent MSG_PING, but is not authenticated or got recently kicked, address = %s:%d", this->getRemoteAddress().to_string().c_str(), this->getRemotePort());
		return;
	}
	m_session->resetTimeoutTimer();

	if (ping.latency() > 0)
		m_session->setLatency(ping.latency());

	lock.unlock();

	Pong pong;
	pong.set_counter(ping.counter());
	packet.setOpcode(MSG_PONG);
	this->packAndSend(std::move(packet), pong);
}

void WorldSocket::packAndSend(WorldPacket&& packet, MessageLite const& message)
{
	try
	{
		packet.pack(message);
		this->queuePacket(std::move(packet));
	}
	catch (PacketException const&)
	{
		NS_LOG_ERROR("world.socket", "Sending to client %s:%d failed because PacketException occurred while packing a packet (opcode: %u)",
			this->getRemoteAddress().to_string().c_str(), this->getRemotePort(), packet.getOpcode());
	}
}

void WorldSocket::handleTimeSyncResp(WorldPacket& recvPacket)
{
	TimeSyncResp timeSync;
	recvPacket.unpack(timeSync);

	std::lock_guard<std::mutex> lock(m_sessMutex);
	if (!m_session)
	{
		NS_LOG_WARN("world.socket", "WorldSocket::handleTimeSyncResp: peer sent CMSG_TIME_SYNC_RESP, but is not authenticated or got recently kicked, address = %s:%d", this->getRemoteAddress().to_string().c_str(), this->getRemotePort());
		return;
	}

	m_session->updateClientTime(timeSync.counter(), timeSync.time());

}


