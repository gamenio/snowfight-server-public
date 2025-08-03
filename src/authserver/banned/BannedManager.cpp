#include "BannedManager.h"

#include "logging/Log.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include "Application.h"


#define BANNED_LIST_FILE  "bannedlist.db"

BannedManager* BannedManager::instance()
{
	static BannedManager instance;
	return &instance;
}

bool BannedManager::load()
{
	std::unordered_map<std::string, BannedAccount> bannedAccounts;
	std::unordered_map<std::string, BannedIP> bannedIPs;

	std::string dbFile = StringUtil::format("%s" BANNED_LIST_FILE, sApplication->getRoot().c_str());
	try
	{
		SQLite::Database db(dbFile);
		try
		{
			SQLite::Statement query(db, "SELECT * FROM account_banned");
			while (query.executeStep())
			{
                std::string playerId = query.getColumn("player_id");
				BannedAccount& account = bannedAccounts[playerId];
				account.playerId = playerId;
				account.banDate = query.getColumn("ban_date");;
			}

		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load banned accounts failed. error: %s", e.what());
			return false;
		}
        
        try
        {
            SQLite::Statement query(db, "SELECT * FROM ip_banned");
            while (query.executeStep())
            {
                std::string ip = query.getColumn("ip");
				BannedIP& account = bannedIPs[ip];
				account.ip = ip;
				account.banDate = query.getColumn("ban_date");;
            }
              
        }
        catch (std::exception& e)
        {
            NS_LOG_INFO("server.loading", "Load banned IPs failed. error: %s", e.what());
            return false;
        }
	}
	catch (std::exception& e)
	{
		NS_LOG_INFO("server.loading", "Failed to load DB file %s. error: %s", dbFile.c_str(), e.what());
		return false;
	}

	std::unique_lock<std::mutex> lock(m_loadMutex);
	m_bannedAccounts = bannedAccounts;
	m_bannedIPs = bannedIPs;

	lock.unlock();

	return true;
}

bool BannedManager::isIPBanned(std::string const& ipAddr)
{
	std::lock_guard<std::mutex> lock(m_loadMutex);

	auto it = m_bannedIPs.find(ipAddr);
	return it != m_bannedIPs.end();
}

bool BannedManager::isAccountBanned(std::string const& playerId)
{
	std::lock_guard<std::mutex> lock(m_loadMutex);

	auto it = m_bannedAccounts.find(playerId);
	return it != m_bannedAccounts.end();
}


BannedManager::BannedManager()
{
}

BannedManager::~BannedManager()
{
}
