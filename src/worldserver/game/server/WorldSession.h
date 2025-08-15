#ifndef __WORLD_SESSION_H__
#define __WORLD_SESSION_H__

#include "protocol/pb/AuthVerdict.pb.h"
#include "protocol/pb/FlashMessage.pb.h"
#include "protocol/pb/PlayerActionMessage.pb.h"

#include "utilities/Timer.h"
#include "containers/ConcurrentQueue.h"
#include "WorldSocket.h"

class WorldSocket;
class Unit;
class Player;
class Theater;

enum RequiredCapabilities
{
	// If a player disconnects without requesting to log out, the server will allow 
	// the player to restore the connection within the timeout time
	REQUIRES_ALLOW_PLAYER_TO_RESTORE		= 0x00000001
};

enum LatencyIndex
{
	LATENCY_LATEST			= 0,
	LATENCY_AVG,
	LATENCY_MIN,
	LATENCY_MAX,
	MAX_LATENCY_INDEXES
};

class WorldSession
{
public:
	enum
	{
		SESSION_TIMEOUT_NEVER	= 0
	};

	explicit WorldSession(std::shared_ptr<WorldSocket> const& socket);
	~WorldSession();

	void addToRecvQueue(WorldPacket&& newPacket);
	void packAndSend(WorldPacket&& packet, MessageLite const& message);

	void setSessionId(uint32 id) { m_sessionId = id; }
	uint32 getSessionId() const { return m_sessionId; }

	void setPlayerId(std::string const& playerId) { m_playerId = playerId; }
	std::string const& getPlayerId() const { return m_playerId; }
	void setOriginalPlayerId(std::string const& playerId) { m_originalPlayerId = playerId; }
	std::string const& getOriginalPlayerId() const { return m_originalPlayerId; }

	uint32 getTheaterId() const;
	Theater* getTheater() const { return m_theater; }

	void setPlayer(Player* player) { m_player = player; }
	Player* getPlayer() const { return m_player; }

	// Set GM permission level
	void setGMLevel(uint8 level) { m_gmLevel = level; }
	uint8 getGMLevel() const { return m_gmLevel; }
	bool hasGMPermission() const { return m_gmLevel > 0; }

	// The capabilities required of the player
	void setRequiredCapabilities(uint32 capabilities) { m_requiredCapabilities = capabilities; }
	uint32 getRequiredCapabilities() const { return m_requiredCapabilities; }
	bool isRequiresCapability(uint32 capability) { return (m_requiredCapabilities & capability) != 0; }

	// Kick the player. The server will immediately disconnect and log out the player 
	// after processing the remaining messages in the receive queue
	void kickPlayer();
	bool isConnected() const { return m_socket && m_socket->isOpen(); }
	// Log out the player and wait for the client to disconnect. 
	// If the wait process times out, the server will actively disconnect
	void logoutPlayer();
	bool isLoggedIn() const { return m_player != nullptr; }
	// Log out the player after the client disconnects, otherwise the player will log out after 
	// the session timeout or after calling the logoutPlayer() function
	void setLogoutAfterDisconnected(bool logout) { m_isLogoutAfterDisconnected = logout; }

	// Update the session. When the session is queued to join the theater, 
	// it is updated by the main thread. Once it joins the theater, it is updated by the theater thread
	bool update(NSTime diff);

	// Login authentication
	void sendAuthVerdict(AuthVerdict::AuthResult result, int32 waitPos = 0);

	// Called when the session is accepted
	void onSessionAccepted();
	void onSessionAccepted(WorldSession* oldSession);
	// Called when the session is expired
	void onSessionExpired();
	// Called when the session is queued to join the theater
	void onSessionQueued(int32 position);
	// Called when the session is accepted by the theater
	void onSessionAcceptedByTheater(Theater* theater);

	// Client latency
	NSTime getLatency(LatencyIndex index);
	void setLatency(NSTime latency);

	// Client IP and port
	std::string const& getRemoteAddress() const { return m_remoteAddress; }
	uint16 getRemotePort() const { return m_remotePort; };

	// Session timeout timer
	void setTimeout(NSTime time);
	NSTime getTimeoutTime() const { return m_timeoutTime; }
	void resetTimeoutTimer();
	void updateTimeoutTimer(NSTime diff);
	bool isSessionTimedOut() const { return m_timeoutTime > 0 && m_timeoutTimer <= 0; }

	// Time synchronization
	NSTime getClientNowTimeMillis();
	void sendTimeSync();
	void resetTimeSync();
	void updateClientTime(int32 counter, int32 time);

	// Send messages to the player
	void sendFlashMessage(FlashMessage const& message);
	void sendPlayerActionMessage(PlayerActionMessage const& message);

	// CharacterHandler
	void sendPlayerLoggedInMesssage();
	void handlePlayerLogin(WorldPacket& recvPacket);
	void handlePlayerLogout(WorldPacket&);
	void handleJoinTheater(WorldPacket& recvPacket);
	void handleQueryCharacterInfo(WorldPacket& recvPacket);
	void sendTheaterInfo();
	void sendCharacterInfo(Unit* unit);
	void handleSmileyChat(WorldPacket& recvPacket);

	// MovementHandler
	void handleMovementInfo(WorldPacket& recvPacket);

	// CombatHandler
	void handleAttackInfo(WorldPacket& recvPacket);
	void handleStaminaInfo(WorldPacket& recvPacket);

	// WorldStatusHandler
	void handleQueryWorldStatus(WorldPacket& recvPacket);
	void sendWorldStatus();
	void handleQueryTheaterStatusList(WorldPacket& recvPacket);
	void sendTheaterStatusList();
	void handleQueryPlayerStatusList(WorldPacket& recvPacket);
	void sendPlayerStatusList(uint32 theaterId);

	// GMCommandHandler
	void handleGMCommand(WorldPacket& recvPacket);

	// ItemHandler
	void handleUseItem(WorldPacket& recvPacket);

private:
	void updateAvgLatency();

	uint32 m_sessionId;
	std::string m_remoteAddress;
	uint16 m_remotePort;

	Theater* m_theater;

	std::shared_ptr<WorldSocket> m_socket;
	ConcurrentQueue<WorldPacket> m_recvQueue;
	bool m_isInQueue;

	bool m_isLoggingOut;
	bool m_isLogoutAfterDisconnected;
	bool m_isAuthed;
	std::string m_playerId;
	std::string m_originalPlayerId;
	Player* m_player;
	uint8 m_gmLevel;
	uint32 m_requiredCapabilities;

	std::mutex m_latencyMutex;
	NSTime m_latencies[MAX_LATENCY_INDEXES];
	uint32 m_totalLatency;
	uint32 m_latencyCounter;

	IntervalTimer m_timeSyncTimer;
	int64 m_timeSyncClient;
	int64 m_timeSyncServer;
	int32 m_timeSyncCounter;
	std::mutex m_timeSyncMutex;

	std::atomic<NSTime> m_timeoutTimer;
	NSTime m_timeoutTime;
};

#endif //__WORLD_SESSION_H__