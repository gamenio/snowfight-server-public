#ifndef __UTIL_H__
#define __UTIL_H__

#include <boost/system/error_code.hpp>

#include "Common.h"

bool isProcessRunning(uint32 pid);

uint32 createPIDFile(std::string const& filename);
uint32 getPIDFromFile(std::string const& filename);
uint32 getPID();

inline std::string getCurrentThreadId()
{
	std::ostringstream oss;
	oss << std::this_thread::get_id();
	return oss.str();
}

void utf8printf(FILE* out, const char *str, ...);
void vutf8printf(FILE* out, const char *str, va_list* ap);

std::string getUtf8ErrorMsg(boost::system::error_code const& error);

#endif // __UTIL_H__
