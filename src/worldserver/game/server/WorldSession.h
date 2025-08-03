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
	// 在玩家未请求登出而直接断开连接的情况下，服务器将在超时时间内允许玩家恢复连接
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

	// 设置GM权限等级
	void setGMLevel(uint8 level) { m_gmLevel = level; }
	uint8 getGMLevel() const { return m_gmLevel; }
	bool hasGMPermission() const { return m_gmLevel > 0; }

	// 玩家需要的功能
	void setRequiredCapabilities(uint32 capabilities) { m_requiredCapabilities = capabilities; }
	uint32 getRequiredCapabilities() const { return m_requiredCapabilities; }
	bool isRequiresCapability(uint32 capability) { return (m_requiredCapabilities & capability) != 0; }

	// 踢出玩家。服务器将立即断开连接，并在处理完接收队列中的剩余消息后登出玩家
	void kickPlayer();
	bool isConnected() const { return m_socket && m_socket->isOpen(); }
	// 登出玩家并等待客户端断开连接，如果等待过程超时服务器将主动断开连接
	void logoutPlayer();
	bool isLoggedIn() const { return m_player != nullptr; }
	// 在客户端断开连接后登出玩家，否则玩家将在会话超时或调用logoutPlayer()函数后登出
	void setLogoutAfterDisconnected(bool logout) { m_isLogoutAfterDisconnected = logout; }

	// 更新会话。当会话排队等待进入战区时会话由主线程更新，当进入战区后由战区线程更新
	bool update(NSTime diff);

	// 登录验证
	void sendAuthVerdict(AuthVerdict::AuthResult result, int32 waitPos = 0);

	// 在会话被接受时调用
	void onSessionAccepted();
	void onSessionAccepted(WorldSession* oldSession);
	// 在会话过期时调用
	void onSessionExpired();
	// 在会话排队加入战区时调用
	void onSessionQueued(int32 position);
	// 在会话被战区接受时调用
	void onSessionAcceptedByTheater(Theater* theater);

	// 客户端延迟
	NSTime getLatency(LatencyIndex index);
	void setLatency(NSTime latency);

	// 客户端IP和端口
	std::string const& getRemoteAddress() const { return m_remoteAddress; }
	uint16 getRemotePort() const { return m_remotePort; };

	// 会话超时计时器
	void setTimeout(NSTime time);
	NSTime getTimeoutTime() const { return m_timeoutTime; }
	void resetTimeoutTimer();
	void updateTimeoutTimer(NSTime diff);
	bool isSessionTimedOut() const { return m_timeoutTime > 0 && m_timeoutTimer <= 0; }

	// 时间同步
	NSTime getClientNowTimeMillis();
	void sendTimeSync();
	void resetTimeSync();
	void updateClientTime(int32 counter, int32 time);

	// 向玩家发送消息
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