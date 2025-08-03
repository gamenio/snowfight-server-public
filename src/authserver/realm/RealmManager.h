#ifndef __REALM_MANAGER_H__
#define __REALM_MANAGER_H__

#include <mutex>

#include "RealmInfo.h"

class RealmManager
{
    typedef std::list<RealmInfo> RealmList;
    typedef std::map<uint32 /* Game version */, RealmList, std::greater<uint32>> VersionedRealms;
	typedef std::map<int32 /* Platform */, VersionedRealms> PlatformedRealms;
    
    struct RealmGroup
    {
        RealmGroup() :
            minRequiredVersion(0)
        { }
        
        uint32 minRequiredVersion;
		PlatformedRealms realms;
    };

public:
	static RealmManager* instance();

	bool load();
	void getRealmList(std::string const& country, uint32 clientVersion, int32 platform, std::list<RealmInfo>& result);
    uint32 getMinRequiredVersion(std::string const& country);

private:
	RealmManager();
	~RealmManager();
	
	std::mutex m_loadMutex;
    std::unordered_map<uint32 /* Group ID */, RealmGroup> m_realmGroups;
    std::unordered_map<std::string /* Country */, uint32 /* Group ID */> m_regionMap;
    std::string m_regionMapDefaultCountry;
};

#define  sRealmManager RealmManager::instance()

#endif // __REALM_MANAGER_H__
