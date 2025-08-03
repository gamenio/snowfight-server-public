#include "WorldSocketMgr.h"

WorldSocketMgr* WorldSocketMgr::instance()
{
	static WorldSocketMgr instance;
	return &instance;
}