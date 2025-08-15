#ifndef __OPCODE_HANDLER_H__
#define __OPCODE_HANDLER_H__

#include "Common.h"
#include "Opcode.h"
#include "game/server/WorldSession.h"

// Declaring WorldSession without importing the header file will cause an OpcodeHandler memory error
// class WorldSession;


enum SessionStatus
{
	STATUS_AUTHED = 0,                                      // The player has been authenticated (m_player == NULL, m_isAuthed)
	STATUS_LOGGEDIN,                                        // The player has logged in to the game (m_player != NULL)
};

struct OpcodeHandler
{
	SessionStatus status;
	void (WorldSession::*handler)(WorldPacket& recvPacket);
};


extern char const* gOpcodeNameTable[NUM_MSG_TYPES];
extern std::unordered_map<uint16/* Opcode */, OpcodeHandler> gOpcodeHandlerTable;


inline const char* lookupOpcodeName(uint16 id)
{
	if (id >= NUM_MSG_TYPES)
		return "Received unknown opcode, it's more than max!";
	return gOpcodeNameTable[id];
}

inline std::string getOpcodeNameForLogging(uint16 opcode)
{
	std::ostringstream ss;
	ss << '[' << lookupOpcodeName(opcode) << " 0x" << std::hex << std::uppercase << opcode << std::nouppercase << " (" << std::dec << opcode << ")]";
	return ss.str();
}


#endif // __OPCODE_HANDLER_H__

