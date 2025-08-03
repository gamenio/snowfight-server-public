#include "RealmManager.h"

#include <boost/algorithm/string.hpp>

#include "utilities/StringUtil.h"
#include "configuration/Config.h"
#include "logging/Log.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include "Application.h"

#define PLATFORM_UNIVERSAL			0


RealmManager* RealmManager::instance()
{
	static RealmManager instance;
	return &instance;
}

bool RealmManager::load()
{
	std::unordered_map<uint32, RealmGroup> realmGroups;
	std::unordered_map<std::string, uint32> regionMap;

	std::string dbFile = StringUtil::format("%srealmlist.db", sApplication->getRoot().c_str());
	try
	{
		SQLite::Database db(dbFile);
		try
		{
			SQLite::Statement query(db, "SELECT * FROM realm_info");
			while (query.executeStep())
			{
                uint32 id = query.getColumn("id");
                uint32 groupId = query.getColumn("group_id");
				int32 platform = query.getColumn("platform");

				// World服务器对客户端游戏版本的兼容性。
				// 例如：World服务器兼容的游戏版本需要大于等于1.0.1，则配置以下字段的值为：
				//    game_version = "1.0.1"
				//    compatibility = 3 (COMPATIBILITY_GREATER|COMPATIBILITY_EQUAL)
				std::string verStr = query.getColumn("game_version");
				uint16 compatibility = query.getColumn("compatibility");

                std::vector<std::string> results;
                boost::split(results, verStr, boost::is_any_of("."));
                if(results.size() > 3)
                    throw std::logic_error(StringUtil::format("'%s' format error in column 'game_version' where id=%u", verStr.c_str(), id));
                uint32 version = 0;
                int32 shift = 16;
                for(auto it = results.begin(); it != results.end(); ++it)
                {
                    std::string pair = *it;
                    uint8 num = static_cast<uint8>(std::stoi(pair));
                    version |= (uint32)num << shift;
                    shift -= 8;
                }
//                NS_LOG_DEBUG(LOG_REALM, "%d.%d.%d (0x%08X)", ((version >> 16) & 0x000000FF), ((version >> 8) & 0x000000FF), (version & 0x000000FF), version);
                RealmGroup& group = realmGroups[groupId];
				if (group.minRequiredVersion <= 0)
					group.minRequiredVersion = version;
				else
					group.minRequiredVersion = std::min(version, group.minRequiredVersion);
				RealmList& realmList = group.realms[platform][version];
				RealmInfo info;
				info.name = query.getColumn("name").getString();
				info.address = query.getColumn("address").getString();
				info.port = query.getColumn("port");
				info.compatibility = compatibility;
				realmList.emplace_back(info);
			}

		}
		catch (std::exception& e)
		{
			NS_LOG_INFO("server.loading", "Load realm info failed. error: %s", e.what());
			return false;
		}
        
        try
        {
            SQLite::Statement query(db, "SELECT * FROM region_map");
            while (query.executeStep())
            {
                std::string country = query.getColumn("country");
                boost::to_upper(country);
                uint32 groupId = query.getColumn("group_id");
                regionMap[country] = groupId;
                if(realmGroups.find(groupId) == realmGroups.end())
                    throw std::logic_error(StringUtil::format("Group ID '%u' is not recorded in 'realm_info' table.", groupId));
            }
            
            if(regionMap.find(m_regionMapDefaultCountry) == regionMap.end())
                throw std::logic_error(StringUtil::format("Default country '%s' was not found in 'region_map' table.", m_regionMapDefaultCountry.c_str()));
                
        }
        catch (std::exception& e)
        {
            NS_LOG_INFO("server.loading", "Load region map failed. error: %s", e.what());
            return false;
        }
	}
	catch (std::exception& e)
	{
		NS_LOG_INFO("server.loading", "Failed to load DB file %s. error: %s", dbFile.c_str(), e.what());
		return false;
	}

	std::unique_lock<std::mutex> lock(m_loadMutex);
	m_realmGroups = realmGroups;
	m_regionMap = regionMap;

	lock.unlock();

	return true;
}

void RealmManager::getRealmList(std::string const& country, uint32 clientVersion, int32 platform,  std::list<RealmInfo>& result)
{
	std::lock_guard<std::mutex> lock(m_loadMutex);

    auto regionIt = m_regionMap.find(country);
    if(regionIt == m_regionMap.end())
        regionIt = m_regionMap.find(m_regionMapDefaultCountry);
    
    if(regionIt != m_regionMap.end())
    {
        auto groupIt = m_realmGroups.find((*regionIt).second);
        if(groupIt != m_realmGroups.end())
        {
			VersionedRealms const* versionedRealms = nullptr;
			PlatformedRealms const& platformedRealms = (*groupIt).second.realms;
			auto it = platformedRealms.find(platform);
			if (it != platformedRealms.end())
				versionedRealms = &(*it).second;
			else
			{
				it = platformedRealms.find(PLATFORM_UNIVERSAL);
				if (it != platformedRealms.end())
					versionedRealms = &(*it).second;
			}
			if (versionedRealms)
			{
				for (auto it = versionedRealms->begin(); it != versionedRealms->end(); ++it)
				{
					uint32 gameVersion = (*it).first;
					std::list<RealmInfo> const& realmList = (*it).second;
					if (clientVersion >= gameVersion)
					{
						for (auto it = realmList.begin(); it != realmList.end(); ++it)
						{
							RealmInfo const& info = *it;
							if (((info.compatibility & COMPATIBILITY_GREATER) == COMPATIBILITY_GREATER && clientVersion > gameVersion)
								|| ((info.compatibility & COMPATIBILITY_EQUAL) == COMPATIBILITY_EQUAL && clientVersion == gameVersion))
								result.push_back(*it);
						}
						break;
					}
				}
			}
        }
    }
}

uint32 RealmManager::getMinRequiredVersion(std::string const& country)
{
	std::lock_guard<std::mutex> lock(m_loadMutex);

    auto regionIt = m_regionMap.find(country);
    if(regionIt == m_regionMap.end())
        regionIt = m_regionMap.find(m_regionMapDefaultCountry);
    
    if(regionIt != m_regionMap.end())
    {
        auto groupIt = m_realmGroups.find((*regionIt).second);
        if(groupIt != m_realmGroups.end())
        {
            RealmGroup const& group = (*groupIt).second;
            return group.minRequiredVersion;
        }
    }

    return 0;
}

RealmManager::RealmManager()
{
    m_regionMapDefaultCountry = sConfigMgr->getStringDefault("RegionMap.DefaultCountry", "EN");
    boost::to_upper(m_regionMapDefaultCountry);
}

RealmManager::~RealmManager()
{
}
