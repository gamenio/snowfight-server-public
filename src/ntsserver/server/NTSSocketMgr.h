#ifndef __NTS_SOCKET_MGR_H__
#define __NTS_SOCKET_MGR_H__

#include "networking/SocketMgr.h"
#include "NTSSocket.h"

class NTSSocketMgr : public SocketMgr<NTSSocket>
{
public:
	static NTSSocketMgr* instance();

protected:
};


#define sNTSSocketMgr NTSSocketMgr::instance()

#endif // __NTS_SOCKET_MGR_H__
