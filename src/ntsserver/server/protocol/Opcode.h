#ifndef __OPCODE_H__
#define __OPCODE_H__

enum Opcode
{
	MSG_NULL						= 0x0000,
	CMSG_TIME_QUERY					= 0x0001,
	SMSG_TIME_RESULT				= 0x0002,
	NUM_MSG_TYPES
};


#endif // __OPCODE_H__