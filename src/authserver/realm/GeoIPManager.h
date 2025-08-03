#ifndef __GEO_IP_MANAGER_H__
#define __GEO_IP_MANAGER_H__

#include "Common.h"

struct MMDB_s;
struct MMDB_entry_s;

struct GeoIPLookupResult
{
	std::string countryCode;
	std::string countryName;
};

class GeoIPManager
{

public:
	static GeoIPManager* instance();

	bool load();
	bool lookupSockaddr(const struct sockaddr *const sockaddr, GeoIPLookupResult& result);

private:
	GeoIPManager();
	~GeoIPManager();

	std::string getStringValue(MMDB_entry_s *const entry, ...);

	MMDB_s* m_mmdb;

};

#define  sGeoIPManager GeoIPManager::instance()


#endif
