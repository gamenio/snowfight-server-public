#ifndef __REALM_CONF_H__
#define __REALM_CONF_H__

#include "Common.h"

enum GameCompatibility
{
	COMPATIBILITY_GREATER	= 0x01,
	COMPATIBILITY_EQUAL		= 0x02,
};

struct RealmInfo
{
	RealmInfo() :
		name(""),
		address(""),
		port(0),
		compatibility(0)
	{
	}


	std::string name;
	std::string address;
	uint16 port;
	uint16 compatibility;
};

#endif // __REALM_CONF_H__