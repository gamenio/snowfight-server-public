#ifndef __NTS_SOCKET_H__
#define __NTS_SOCKET_H__

#include "networking/Socket.h"
#include "protocol/Opcode.h"


typedef BasicPacket<NUM_MSG_TYPES> NTSPacket;

class NTSSocket : public Socket<NTSSocket, NTSPacket>
{
	typedef Socket<NTSSocket, NTSPacket> BasicSocket;

public:
	NTSSocket(tcp::socket&& socket);
	~NTSSocket();

	void start() override;
	void onReceivedData(NTSPacket&& packet) override;
	void onSocketClosed() override;
	void update() override;

private:
	void checkIP();
	void packAndSend(NTSPacket&& packet, MessageLite const & message);

	void resetConnectionTimeout();

	void sendTimeResult(int64 originateTimestamp);
	void handleTimeQuery(NTSPacket& packet);

	int32 m_connTimeout;
	int64 m_connActiveTime;
};

#endif // __NTS_SOCKET_H__