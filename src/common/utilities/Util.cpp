#include "Util.h"

#include <stdarg.h>

#include "StringUtil.h"

#if PLATFORM == PLATFORM_WINDOWS
#include <Windows.h>
#include <Psapi.h>

static const int UTF8PRINT_BUFFER_SIZE = 16 * 1024;

#endif

bool isProcessRunning(uint32 pid)
{
	bool isRunning = false;
#ifdef _WIN32
	if (HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid))
	{
		TCHAR filename[MAX_PATH];
		if (GetModuleFileNameEx(process, NULL, filename, MAX_PATH))
		{
			if (HANDLE currProcess = GetCurrentProcess())
			{
				TCHAR currFilename[MAX_PATH];
				if (GetModuleFileNameEx(currProcess, NULL, currFilename, MAX_PATH))
				{
					isRunning = strcmp(filename, currFilename) == 0;
				}
				CloseHandle(currProcess);
			}
		}
		CloseHandle(process);
	}

#else

	if(kill((pid_t)pid, 0) == 0)
	{
		char path[PATH_MAX];
		sprintf(path, "/proc/%d/exe", pid);
		char procLink[PATH_MAX];
		if(readlink(path, procLink, PATH_MAX) != -1)
		{
			char currProcLink[PATH_MAX];
			sprintf(path, "/proc/%d/exe", getpid());
			if(readlink(path, currProcLink, PATH_MAX) != -1)
			{
				isRunning = strstr(procLink, currProcLink) != NULL;
			}
		}
	}

#endif
	return isRunning;
}

uint32 getPIDFromFile(std::string const& filename)
{
	uint32 pid = 0;
	FILE* pid_file = fopen(filename.c_str(), "r");
	if (pid_file != NULL)
	{
		char line[16];
		if (fgets(line, 16, pid_file) != NULL)
			pid = (uint32)strtoul(line, NULL, 10);
		fclose(pid_file);
	}
	return pid;
}

/// create PID file
uint32 createPIDFile(std::string const& filename)
{
	FILE* pid_file = fopen(filename.c_str(), "w");
	if (pid_file == NULL)
		return 0;

	uint32 pid = getPID();

	fprintf(pid_file, "%u", pid);
	fclose(pid_file);

	return pid;
}

uint32 getPID()
{
#ifdef _WIN32
	DWORD pid = GetCurrentProcessId();
#else
	pid_t pid = getpid();
#endif

	return uint32(pid);
}

void utf8printf(FILE* out, const char *str, ...)
{
    va_list ap;
    va_start(ap, str);
    vutf8printf(out, str, &ap);
    va_end(ap);
}

void vutf8printf(FILE* out, const char *str, va_list* ap)
{
#if PLATFORM == PLATFORM_WINDOWS
	int bufferSize = UTF8PRINT_BUFFER_SIZE;
	char* buf = nullptr;
	int nret = 0;
	do
	{
		buf = new (std::nothrow) char[bufferSize];
		if (buf == nullptr)
			return;
		/*
		pitfall: The behavior of vsnprintf between VS2013 and VS2015/2017 is different
		VS2013 or Unix-Like System will return -1 when buffer not enough, but VS2015/2017 will return the actural needed length for buffer at this station
		The _vsnprintf behavior is compatible API which always return -1 when buffer isn't enough at VS2013/2015/2017
		Yes, The vsnprintf is more efficient implemented by MSVC 19.0 or later, AND it's also standard-compliant, see reference: http://www.cplusplus.com/reference/cstdio/vsnprintf/
		*/
		nret = vsnprintf(buf, bufferSize, str, *ap);
		if (nret >= 0)
		{ // VS2015/2017
			if (nret < bufferSize)
			{// success, so don't need to realloc
				break;
			}
			else
			{
				bufferSize = nret + 1;
				delete[] buf;
			}
		}
		else // < 0
		{	// VS2013 or Unix-like System(GCC)
			bufferSize *= 2;
			delete[] buf;
		}
	} while (true);
	assert(buf[nret] == '\0');

	int pos = 0;
	int len = nret;
	char tempBuf[UTF8PRINT_BUFFER_SIZE + 1] = { 0 };
	WCHAR wszBuf[UTF8PRINT_BUFFER_SIZE + 1] = { 0 };

	do
	{
		std::copy(buf + pos, buf + pos + UTF8PRINT_BUFFER_SIZE, tempBuf);

		tempBuf[UTF8PRINT_BUFFER_SIZE] = 0;
		DWORD nChar = MultiByteToWideChar(CP_UTF8, 0, tempBuf, -1, wszBuf, sizeof(wszBuf));
		if (nChar)
			CharToOemBuffW(wszBuf, tempBuf, nChar);
		fprintf(out, "%s", tempBuf);

		pos += UTF8PRINT_BUFFER_SIZE;

	} while (pos < len);
#else
    vfprintf(out, str, *ap);
#endif
}

std::string getUtf8ErrorMsg(boost::system::error_code const& error)
{
	std::string emsg = error.message();

	// When BOOST_SYSTEM_USE_UTF8 is defined, the Boost library on Windows uses code page CP_UTF8 
	// instead of the default CP_ACP and returns UTF-8 messages. This macro has no affect on POSIX.
#if PLATFORM == PLATFORM_WINDOWS && !defined(BOOST_SYSTEM_USE_UTF8)
	if (!emsg.empty())
	{
		std::wstring wstr = StringUtil::ansiToWideChar(emsg);
		if (!wstr.empty())
		{
			std::string utf8str = StringUtil::wideCharToUtf8(wstr);
			if (!utf8str.empty())
				return utf8str;
		}

	}
#endif

	return emsg;
}