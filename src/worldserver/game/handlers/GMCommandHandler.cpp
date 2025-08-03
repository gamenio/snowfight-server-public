#include "game/server/WorldSession.h"

#include "game/server/protocol/pb/GMCommand.pb.h"
#include "GMCommandWorker.h"

void WorldSession::handleGMCommand(WorldPacket& recvPacket)
{
	if (!this->hasGMPermission())
		return;

	GMCommand message;
	recvPacket.unpack(message);

	std::string line = message.line();
	if(!line.empty())
		GMCommandWorker(this).parseCommand(line);
}
