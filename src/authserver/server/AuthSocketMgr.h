#ifndef __AUTH_SOCKET_MGR_H__
#define __AUTH_SOCKET_MGR_H__

#include "networking/SocketMgr.h"
#include "AuthSocket.h"

class AuthSocketMgr : public SocketMgr<AuthSocket>
{
public:
	static AuthSocketMgr* instance();

protected:
};


#define sAuthSocketMgr AuthSocketMgr::instance()

#endif //__AUTH_SOCKET_MGR_H__
