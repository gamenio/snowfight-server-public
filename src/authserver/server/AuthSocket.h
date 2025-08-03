#ifndef __AUTH_SOCKET_H__
#define __AUTH_SOCKET_H__

#include "networking/Socket.h"
#include "protocol/Opcode.h"

class AuthSession;


typedef BasicPacket<NUM_MSG_TYPES> AuthPacket;

class AuthSocket : public Socket<AuthSocket, AuthPacket>
{
	typedef Socket<AuthSocket, AuthPacket> BasicSocket;

public:
	AuthSocket(tcp::socket&& socket);
	~AuthSocket();

	void start() override;
	void onReceivedData(AuthPacket&& packet) override;
	void onSocketClosed() override;
	void update() override;

private:
	void checkIP();
	void packAndSend(AuthPacket&& packet, MessageLite const & message);

	void resetSessionTimeout();

	AuthSession* m_session;

	int32 m_sessTimeout;
	int64 m_sessActiveTime;
	int32 m_latency;
};

#endif //__AUTH_SOCKET_H__