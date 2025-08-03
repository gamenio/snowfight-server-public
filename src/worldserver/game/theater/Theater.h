#ifndef __THEATER_H__
#define __THEATER_H__

#include "Common.h"
#include "utilities/Timer.h"
#include "game/server/WorldSession.h"
#include "game/maps/BattleMap.h"
#include "game/maps/SpawnManager.h"

class Theater
{
	typedef std::unordered_map<uint32 /* SessionId */, WorldSession*> SessionMap;
	typedef std::vector<WorldSession*> SessionList;

	enum State
	{
		STATE_IDLE,
		STATE_WAITING_FOR_PLAYERS,
		STATE_PLAYERS_IN_PLACE,
		STATE_ACTIVE,
	};

public:
	Theater(uint32 theaterId);
	~Theater();

	void addSession(WorldSession* session);
	bool acceptSession(WorldSession* session);
	bool removeSession(uint32 id);
	// 替换具有相同SessionID的会话
	// 如果替换成功则返回true，否则返回false
	bool replaceSession(WorldSession* session);
	SessionMap& getSessionList() { return m_sessions; };
	SessionMap const& getSessionList() const { return m_sessions; };

	// 等待玩家。如果需要等待则返回true，否则返回false。
	bool waitForPlayers();
	void skipWaitingPlayers();
	void sendWaitForPlayersToPlayer(WorldSession* session);
	void setWaitForPlayersTimeout(NSTime time);

	float getOrder(WorldSession* session) const;

	int32 getOnlineCount() const { return m_playerCount + m_gmCount; }
	int32 getPlayerCount() const { return m_playerCount; }
	int32 getGMCount() const { return m_gmCount; }
	int32 getPopulationCap() const { return m_map->getMapData()->getPopulationCap(); }

	uint32 getTheaterId() const { return m_theaterId; }
	uint32 getBattleCount() const { return m_battleCount; }
	BattleMap* getMap() const { return m_map; }
	SpawnManager* getSpawnManager() const { return m_spawnManager; }

	void advancedUpdate(NSTime diff);
	void update(NSTime diff);
	NSTime getUpdateDiff() const { return m_updateDiff; }

	void setDeletionDelay(NSTime delay);
	bool canDelete() const;
	bool isDeleting() const { return m_isDeleting; }

private:
	void deleteDelayed();
	void undelete();

	bool createMapIfNotExistForPlayer(WorldSession* session);
	void configureMapForPlayer(WorldSession* session);

	bool isSleepState() const { return this->getOnlineCount() <= 0 && m_state == STATE_IDLE; }

	uint32 m_theaterId;
	uint32 m_battleCount;
	SessionMap m_sessions;
	int32 m_playerCount;
	int32 m_gmCount;

	State m_state;
	DelayTimer m_waitForPlayersTimer;

	BattleMap* m_map;
	SpawnManager* m_spawnManager;
	NSTime m_updateDiff;

	bool m_isDeleting;
	NSTime m_deletionDelay;
	int64 m_deletionStartTime;
};

#endif //__THEATER_H__
