#ifndef __OPCODE_HANDLER_H__
#define __OPCODE_HANDLER_H__

#include "Common.h"
#include "Opcode.h"
#include "server/AuthSession.h"

// 仅对AuthSession进行前置声明而不导入头文件会导致OpcodeHandler内存错误
// class AuthSession;


struct OpcodeHandler
{
	void (AuthSession::*handler)(AuthPacket& recvPacket);
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


#endif //__OPCODE_HANDLER_H__

