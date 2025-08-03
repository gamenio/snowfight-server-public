#include "OpcodeHandler.h"

// #include "game/server/AuthSession.h"


// Auth Opcode Names
char const* gOpcodeNameTable[NUM_MSG_TYPES] = {
	"MSG_NULL",
	"CMSG_LOGON_CHALLENGE",
	"SMSG_LOGON_RESULT",
	"CMSG_GET_REALM_LIST"
	"SMSG_REALM_LIST",
};

// Auth Opcode Handlers
std::unordered_map<uint16, OpcodeHandler> gOpcodeHandlerTable = 
{
	{ CMSG_LOGON_CHALLENGE,			{ &AuthSession::handleLogonChallenge	} },
	{ CMSG_GET_REALM_LIST,			{ &AuthSession::handleGetRealmList		} },
};

