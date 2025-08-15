#include "AuthSession.h"

#include "protocol/OpcodeHandler.h"
#include "AuthSocket.h"


AuthSession::AuthSession(AuthSocket* socket):
	m_sessionId(0),
	m_remoteAddress(socket->getRemoteAddress().to_string()),
	m_remotePort(socket->getRemotePort()),
	m_socket(socket),
	m_gameVersion(0),
	m_platform(Platform::PLATFORM_UNKNOWN),
	m_requiredCapabilities(0)
{
}


AuthSession::~AuthSession()
{
	m_socket = nullptr;
}


void AuthSession::handlePacket(AuthPacket&& newPacket)
{
	try
	{
		// Call the Handler corresponding to the Opcode to process the protocol data
		if (gOpcodeHandlerTable.find(newPacket.getOpcode()) != gOpcodeHandlerTable.end())
		{
			OpcodeHandler& opHandler = gOpcodeHandlerTable[newPacket.getOpcode()];
			(this->*opHandler.handler)(newPacket);
		}
		else
			NS_LOG_ERROR("auth.session", "Received unhandled opcode %s from %s:%d"
				, getOpcodeNameForLogging(newPacket.getOpcode()).c_str(), m_remoteAddress.c_str(), m_remotePort);
	}
	catch (PacketException const& ex)
	{
		NS_LOG_ERROR("auth.session", "PacketException occurred while AuthSession::handlePacket call OpcodeHandler(opcode: 0x%04X) from client %s:%d, sessionid=%u. Skipped packet.\n%s",
			newPacket.getOpcode(), m_remoteAddress.c_str(), m_remotePort, this->getSessionId(), ex.what());
	}
}


void AuthSession::packAndSend(AuthPacket&& packet, MessageLite const& message)
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
		NS_LOG_ERROR("auth.session", "Sending to client %s:%d failed because PacketException occurred while packing a packet (opcode: %u), sessionid=%u. errmsg: %s",
			m_remoteAddress.c_str(), m_remotePort, packet.getOpcode(), this->getSessionId(), ex.what());
	}																			
}


void AuthSession::kick()
{
	if (m_socket)
		m_socket->closeSocket();

	m_socket = nullptr;
}

std::string AuthSession::getNetworkTypeName(NetworkType type) {
	switch (type) 
	{
	case NETWORK_TYPE_2G:
		return "2G";
	case NETWORK_TYPE_3G:
		return "3G";
	case NETWORK_TYPE_4G:
		return "4G";
	case NETWORK_TYPE_5G:
		return "5G";
	case NETWORK_TYPE_WIFI:
		return "WiFi";
	default: // NETWORK_TYPE_UNKNOWN
		return "UNKNOWN";
	}
}

