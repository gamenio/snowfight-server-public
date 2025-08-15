#ifndef __THEATER_MANAGER_H__
#define __THEATER_MANAGER_H__

#include <boost/asio/io_service.hpp>

#include "Common.h"
#include "utilities/Timer.h"
#include "game/server/WorldSession.h"
#include "Theater.h"
#include "TheaterUpdater.h"

class TheaterManager
{
	typedef std::unordered_map<uint32, WorldSession*> SessionMap;
	typedef std::unordered_map<uint32, Theater*> TheaterMap;
	typedef std::vector<Theater*> OrderedTheaterList;
	typedef std::list<WorldSession*> SessionList;

public:
	static TheaterManager* instance();

	void initialize();
	// Stop the theater service
	void stop();
	bool isStopped() const { return m_isStopped; }

	void queueSession(WorldSession* session);
	// Find accepted sessions by ID
	WorldSession const* findSession(uint32 id) const;

	int32 getOnlineCount() const { return m_playerCount + m_gmCount; }
	int32 getPlayerLimit() const { return m_playerLimit; }
	int32 getPlayerCount() const { return m_playerCount; }
	int32 getQueuedPlayerCount() const { return static_cast<int32>(m_queuedPlayers.size()); }
	int32 getGMCount() const { return m_gmCount; }

	Theater const* findTheater(uint32 id) const;
	std::vector<Theater*> const& getTheaterList() const { return m_orderedTheaters; }
	int32 getTheaterCount() const { return static_cast<int32>(m_orderedTheaters.size()); }
	void addPlayerToTheater(WorldSession* session);
	uint16 selectMapForPlayer(WorldSession* session) const;

	// Returns false when the theater service is completely stopped
	bool update(NSTime diff);

private:
	TheaterManager();
	~TheaterManager();

	uint32 generateSessionId();

	Theater* createTheater();
	Theater* selectTheaterForPlayer(WorldSession* session);
	void purgeTheaters(bool force);
	void updateTheaters(NSTime diff);

	void processPendingSessions();
	void kickAll();
	bool tryAddSession(WorldSession* session);
	bool tryRestoreSession(WorldSession* session);
	void addGMSession(WorldSession* session);
	void updateSessions(NSTime diff);

	void updateQueuedPlayers(NSTime diff);
	int32 getQueuedPosition(WorldSession* session);
	void addQueuedPlayer(WorldSession* session);

	void updateExpiredPlayers(NSTime diff);
	void addExpiredPlayer(WorldSession* session);

	std::atomic<bool> m_isStopping;
	std::atomic<bool> m_isStopped;

	TheaterUpdater m_updater;
	TheaterMap m_theaters;
	OrderedTheaterList m_orderedTheaters;
	int32 m_playerLimit;
	int32 m_playerCount;
	int32 m_gmCount;
	NSTime m_theaterDeletionDelay;
	NSTime m_waitForPlayersTimeout;

	ConcurrentQueue<WorldSession*> m_pendingSessions;
	SessionMap m_sessions;
	SessionList m_queuedPlayers;
	SessionList m_expiredPlayers;

	NSTime m_sessionTimeout;
	NSTime m_expiredSessionDelay;
	NSTime m_queuedSessionTimeout;
};

#define sTheaterManager TheaterManager::instance()

#endif //__THEATER_MANAGER_H__
