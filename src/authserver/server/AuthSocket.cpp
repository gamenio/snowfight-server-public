#include "AuthSocket.h"

#include "protocol/Opcode.h"
#include "AuthSession.h"
#include "utilities/Util.h"
#include "utilities/TimeUtil.h"
#include "Application.h"


AuthSocket::AuthSocket(tcp::socket&& socket) :
	Socket(std::move(socket)),
	m_session(nullptr),
	m_sessTimeout(0),
	m_sessActiveTime(0),
	m_latency(0)
{
	m_sessTimeout = sApplication->getIntConfig(CONFIG_SESSION_TIMEOUT);
}

AuthSocket::~AuthSocket()
{
	this->closeSocket();

	if (m_session)
	{
		delete m_session;
		m_session = nullptr;
	}
}

void AuthSocket::start()
{
	this->checkIP();
}


void AuthSocket::onReceivedData(AuthPacket&& packet)
{
	this->resetSessionTimeout();

	try
	{
		if (m_session)
		{
			m_session->handlePacket(std::move(packet));
		}
	}
	catch (PacketException const&)
	{
		NS_LOG_ERROR("auth.socket", "AuthSocket::onReceivedData PacketException occurred while unpacking a packet (opcode: %u) from client %s:%d",
			packet.getOpcode(), this->getRemoteAddress().to_string().c_str(), this->getRemotePort());
	}
}


void AuthSocket::onSocketClosed()
{
	if (m_session)
	{
		delete m_session;
		m_session = nullptr;
	}

	NS_LOG_INFO("auth.socket", "Session from client %s:%d is closed. latency: %dms", this->getRemoteAddress().to_string().c_str(), this->getRemotePort(), m_latency);
}


void AuthSocket::update()
{
	BasicSocket::update();

	// 判断会话是否超时
	if (m_sessTimeout > 0 && m_sessActiveTime > 0)
	{
		int32 diff = (int32)(getUptimeMillis() - m_sessActiveTime);
		if (diff > m_sessTimeout)
		{
			NS_LOG_WARN("auth.socket", "Session timed out from client %s:%d.", this->getRemoteAddress().to_string().c_str(), this->getRemotePort());
			m_latency = 0;
			this->closeSocket();
		}
	}
}

void AuthSocket::checkIP()
{
	// TODO 检查IP是否在黑名单

	if (m_session)
	{
		NS_LOG_ERROR("auth.socket", "Session from client %s:%d is repeatedly created.", this->getRemoteAddress().to_string().c_str(), this->getRemotePort());
		this->closeSocket();
	}
	else
	{
		m_session = new AuthSession(this);
		this->asyncRead();
		this->resetSessionTimeout();
	}
}


void AuthSocket::packAndSend(AuthPacket&& packet, MessageLite const& message)
{
	try
	{
		packet.pack(message);
		this->queuePacket(std::move(packet));
	}
	catch (PacketException const&)
	{
		NS_LOG_ERROR("auth.socket", "Sending to client %s:%d failed because PacketException occurred while packing a packet (opcode: %u)",
			this->getRemoteAddress().to_string().c_str(), this->getRemotePort(), packet.getOpcode());
	}
}

void AuthSocket::resetSessionTimeout()
{
	if (m_sessTimeout <= 0)
		return;

	int64 currTime = getUptimeMillis();
	if (m_sessActiveTime > 0)
		m_latency = (int32)(currTime - m_sessActiveTime);

	m_sessActiveTime = currTime;
}

