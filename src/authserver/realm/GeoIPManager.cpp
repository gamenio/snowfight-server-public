#include "GeoIPManager.h"

#include "maxminddb.h"

#include "logging/Log.h"
#include "../Application.h"


#define MMDB_FILE			"geoip.mmdb"

GeoIPManager* GeoIPManager::instance()
{
	static GeoIPManager instance;
	return &instance;
}

GeoIPManager::GeoIPManager() :
	m_mmdb(NULL)
{
}

GeoIPManager::~GeoIPManager()
{
	if (m_mmdb)
	{
		MMDB_close(m_mmdb);
		free(m_mmdb);
		m_mmdb = NULL;
	}

}

std::string GeoIPManager::getStringValue(MMDB_entry_s *const entry,  ...)
{
	MMDB_entry_data_s entryData;
	va_list path;
	va_start(path, entry);
	int32 status = MMDB_vget_value(entry, &entryData, path);
	va_end(path);
	if (status == MMDB_SUCCESS)
	{
		if (entryData.type == MMDB_DATA_TYPE_UTF8_STRING)
		{
			return std::string(entryData.utf8_string, entryData.data_size);
		}
		else
			status = MMDB_INVALID_DATA_ERROR;
	}
	
	NS_LOG_DEBUG("auth.geoip", "Got an error looking up the entry data - %s", MMDB_strerror(status));

	return "";
}


bool GeoIPManager::load()
{
	char dbFile[NS_PATH_MAX];
	snprintf(dbFile, NS_PATH_MAX, "%s%s", sApplication->getRoot().c_str(), MMDB_FILE);

	m_mmdb = (MMDB_s *)malloc(sizeof(MMDB_s));
	NS_ASSERT_ERROR(m_mmdb != NULL, "Could not allocate memory for MMDB_s struct");

	int32 status = MMDB_open(dbFile, MMDB_MODE_MMAP, m_mmdb);
	if (status == MMDB_SUCCESS)
	{
		return true;
	}

	MMDB_close(m_mmdb);
	free(m_mmdb);
	m_mmdb = NULL;

	NS_LOG_ERROR("server.loading", "Can't open %s - %s", dbFile, MMDB_strerror(status));

	return false;
}

bool GeoIPManager::lookupSockaddr(const struct sockaddr *const sockaddr, GeoIPLookupResult& result)
{
	int32 mmdbError;
	MMDB_lookup_result_s mmdbRet = MMDB_lookup_sockaddr(m_mmdb, sockaddr, &mmdbError);
	if (mmdbError == MMDB_SUCCESS)
	{
		if (mmdbRet.found_entry)
		{
			result.countryCode = this->getStringValue(&mmdbRet.entry, "country", "iso_code", NULL);
			result.countryName = this->getStringValue(&mmdbRet.entry, "country", "names", "en", NULL);
			return true;
		}
	}
	else
		NS_LOG_DEBUG("auth.geoip", "Got an error from libmaxminddb: %s", MMDB_strerror(mmdbError));

	return false;
}
