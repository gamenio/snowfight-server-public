#include "NTSSocket.h"

#include "protocol/pb/TimeQuery.pb.h"
#include "protocol/pb/TimeResult.pb.h"

#include "protocol/Opcode.h"
#include "utilities/Util.h"
#include "utilities/TimeUtil.h"
#include "Application.h"


NTSSocket::NTSSocket(tcp::socket&& socket) :
	Socket(std::move(socket)),
	m_connTimeout(0),
	m_connActiveTime(0)
{
	m_connTimeout = sApplication->getIntConfig(CONFIG_CONNECTION_TIMEOUT);
}

NTSSocket::~NTSSocket()
{
	this->closeSocket();
}

void NTSSocket::start()
{
	this->checkIP();
}


void NTSSocket::onReceivedData(NTSPacket&& packet)
{
	this->resetConnectionTimeout();
	try
	{
		switch (packet.getOpcode())
		{
		case CMSG_TIME_QUERY:
			this->handleTimeQuery(packet);
			break;
		default:
			break;
		}
	}
	catch (PacketException const&)
	{
		NS_LOG_ERROR("nts.socket", "NTSSocket::onReceivedData PacketException occurred while unpacking a packet (opcode: %u) from client %s:%d",
			packet.getOpcode(), this->getRemoteAddress().to_string().c_str(), this->getRemotePort());
	}
}


void NTSSocket::onSocketClosed()
{
	NS_LOG_INFO("nts.socket", "Connection from client %s:%d is closed.", this->getRemoteAddress().to_string().c_str(), this->getRemotePort());
}


void NTSSocket::update()
{
	BasicSocket::update();

	// Check if the connection has timed out
	if (m_connTimeout > 0 && m_connActiveTime > 0)
	{
		int32 diff = (int32)(getUptimeMillis() - m_connActiveTime);
		if (diff > m_connTimeout)
		{
			NS_LOG_WARN("nts.socket", "Connection timed out from client %s:%d.", this->getRemoteAddress().to_string().c_str(), this->getRemotePort());
			this->closeSocket();
		}
	}
}

void NTSSocket::checkIP()
{
	this->asyncRead();
	this->resetConnectionTimeout();
}


void NTSSocket::packAndSend(NTSPacket&& packet, MessageLite const& message)
{
	try
	{
		packet.pack(message);
		this->queuePacket(std::move(packet));
	}
	catch (PacketException const&)
	{
		NS_LOG_ERROR("nts.socket", "Sending to client %s:%d failed because PacketException occurred while packing a packet (opcode: %u)",
			this->getRemoteAddress().to_string().c_str(), this->getRemotePort(), packet.getOpcode());
	}
}

void NTSSocket::resetConnectionTimeout()
{
	if (m_connTimeout <= 0)
		return;

	m_connActiveTime = getUptimeMillis();
}


void NTSSocket::sendTimeResult(int64 originateTimestamp)
{
	TimeResult result;
	int64 currTime = getSystemTimeMillis();

	// The time (t1) when the client sends the request
	result.set_originate_timestamp(originateTimestamp);
	// The time (t2) when the server receives the request
	result.set_receive_timestamp(currTime);
	// The time (t3) when the server sends the response 
	result.set_transmit_timestamp(currTime);

	NTSPacket packet(SMSG_TIME_RESULT);
	this->packAndSend(std::move(packet), result);
}

void NTSSocket::handleTimeQuery(NTSPacket& packet)
{
	TimeQuery result;
	packet.unpack(result);

	NS_LOG_INFO("nts.socket", "Time query from client %s:%d, timestamp(ms): " SI64FMTD, this->getRemoteAddress().to_string().c_str(), this->getRemotePort(), result.transmit_timestamp());

	this->sendTimeResult(result.transmit_timestamp());
}