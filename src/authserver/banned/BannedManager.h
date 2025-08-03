#ifndef __BANNED_MANAGER_H__
#define __BANNED_MANAGER_H__

#include <mutex>

#include "Common.h"

class BannedManager
{
	struct BannedAccount
	{
		std::string playerId;
		uint32 banDate;
	};

	struct BannedIP
	{
		std::string ip;
		uint32 banDate;
	};

public:
	static BannedManager* instance();

	bool load();
	
	bool isIPBanned(std::string const& ipAddr);
	bool isAccountBanned(std::string const& playerId);

private:
	BannedManager();
	~BannedManager();
	
	std::mutex m_loadMutex;
    std::unordered_map<std::string /* Player ID */, BannedAccount> m_bannedAccounts;
    std::unordered_map<std::string /* IP */, BannedIP> m_bannedIPs;
};

#define sBannedManager BannedManager::instance()

#endif // __BANNED_MANAGER_H__
