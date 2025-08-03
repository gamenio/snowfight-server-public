#ifndef __OPCODE_H__
#define __OPCODE_H__

enum Opcode
{
	MSG_NULL						= 0x0000,
	CMSG_LOGON_CHALLENGE			= 0x0001,
	SMSG_LOGON_RESULT				= 0x0002,
	CMSG_GET_REALM_LIST				= 0x0003,
	SMSG_REALM_LIST					= 0x0004,
	NUM_MSG_TYPES
};


#endif //__OPCODE_H__