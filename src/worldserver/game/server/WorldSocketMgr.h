#ifndef __WORLD_SOCKET_MGR_H__
#define __WORLD_SOCKET_MGR_H__

#include "networking/SocketMgr.h"
#include "WorldSocket.h"

class WorldSocketMgr : public SocketMgr<WorldSocket>
{
public:
	static WorldSocketMgr* instance();

protected:
};


#define sWorldSocketMgr WorldSocketMgr::instance()

#endif //__WORLD_SOCKET_MGR_H__
