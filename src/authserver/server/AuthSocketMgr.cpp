#include "AuthSocketMgr.h"

AuthSocketMgr* AuthSocketMgr::instance()
{
	static AuthSocketMgr instance;
	return &instance;
}