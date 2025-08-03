#include "NTSSocketMgr.h"

NTSSocketMgr* NTSSocketMgr::instance()
{
	static NTSSocketMgr instance;
	return &instance;
}