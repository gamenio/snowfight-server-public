#ifndef __WORLD_SOCKET_H__
#define __WORLD_SOCKET_H__

#include "networking/Socket.h"
#include "protocol/Opcode.h"

class WorldSession;

typedef BasicPacket<NUM_MSG_TYPES> WorldPacket;

class WorldSocket : public Socket<WorldSocket, WorldPacket>
{
public:
	WorldSocket(tcp::socket&& socket);
	~WorldSocket();

	void start() override;
	void onReceivedData(WorldPacket&& packet) override;
	void onSocketClosed() override;

private:
	void checkIP();
	void packAndSend(WorldPacket&& packet, MessageLite const & message);

	// Login authentication
	//void sendAuthChallenge();
	void handleAuthProof(WorldPacket& packet);

	// Ping
	void handlePing(WorldPacket& packet);

	// Time synchronization
	void handleTimeSyncResp(WorldPacket & recvPacket);

	WorldSession* m_session;
	std::mutex m_sessMutex;


};

#endif //__WORLD_SOCKET_H__