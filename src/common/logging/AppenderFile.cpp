#include "AppenderFile.h"

#include <algorithm>

#include <boost/filesystem.hpp>

#include "Common.h"
#include "utilities/TimeUtil.h"
#include "utilities/StringUtil.h"
#include "Log.h"

#if PLATFORM == PLATFORM_WINDOWS
# include <Windows.h>
#endif

namespace fs = boost::filesystem;

AppenderFile::AppenderFile(uint8 id, std::string const& name, LogLevel level, AppenderFlags flags, ExtraAppenderArgs extraArgs) :
    Appender(id, name, level, flags),
    m_logfile(NULL),
    m_logDir(sLog->getLogsDir()),
    m_fileSize(0)
{
    if (extraArgs.empty())
        throw InvalidAppenderArgsException(StringUtil::format("Log::CreateAppenderFromConfig: Missing file name for appender %s\n", name.c_str()));

    m_fileName = extraArgs[0];

    char const* mode = "a";
    if (extraArgs.size() > 1)
        mode = extraArgs[1];

    if (flags & APPENDER_FLAGS_USE_TIMESTAMP)
    {
        size_t dot_pos = m_fileName.find_last_of(".");
        if (dot_pos != std::string::npos)
            m_fileName.insert(dot_pos, sLog->getLogsTimestamp());
        else
            m_fileName += sLog->getLogsTimestamp();
    }

    if (extraArgs.size() > 2)
        m_maxFileSize = atoi(extraArgs[2]);
    else
        m_maxFileSize = 0;

    m_dynamicName = std::string::npos != m_fileName.find("%s");
    m_backup = (flags & APPENDER_FLAGS_MAKE_FILE_BACKUP) != 0;

    if (!m_dynamicName)
        m_logfile = openFile(m_fileName, mode, !strcmp(mode, "w") && m_backup);
}

AppenderFile::~AppenderFile()
{
    closeFile();
}

void AppenderFile::_write(LogMessage const* message)
{
	bool exceedMaxSize = m_maxFileSize > 0 && (m_fileSize + message->size()) > m_maxFileSize;

    if (m_dynamicName)
    {
        char namebuf[NS_PATH_MAX];
        snprintf(namebuf, NS_PATH_MAX, m_fileName.c_str(), message->m_param1.c_str());
        // always use "a" with dynamic name otherwise it could delete the log we wrote in last _write() call
        FILE* file = openFile(namebuf, "a", m_backup || exceedMaxSize);
        if (!file)
            return;
        fprintf(file, "%s%s\n", message->m_prefix.c_str(), message->m_text.c_str());
        fflush(file);
        m_fileSize += uint64(message->size());
        fclose(file);
        return;
    }
	else if (exceedMaxSize)
		m_logfile = openFile(m_fileName, "w", true);

    if (!m_logfile)
        return;

    fprintf(m_logfile, "%s%s\n", message->m_prefix.c_str(), message->m_text.c_str());
	fflush(m_logfile);
	m_fileSize += uint64(message->size());
}

FILE* AppenderFile::openFile(std::string const& filename, std::string const& mode, bool backup)
{
	std::string fullName(m_logDir + filename);
	try
	{
		fs::path fullPath = fs::path(fullName).parent_path();
		if (!fullPath.empty() && !fs::exists(fullPath))
			fs::create_directories(fullPath);
	}
	catch (std::exception const& ex)
	{
		fprintf(stderr, "AppenderFile::openFile: %s\n", ex.what());
		return NULL;
	}

    if (backup)
    {
        closeFile();
        std::string newName(fullName);
		std::string timeStr = Log::getTimestampStr();
        newName.push_back('.');
        newName.append(timeStr);
        std::rename(fullName.c_str(), newName.c_str()); // no error handling... if we couldn't make a backup, just ignore
    }

    if (FILE* ret = fopen(fullName.c_str(), mode.c_str()))
    {
        m_fileSize = ftell(ret);
        return ret;
    }

    return NULL;
}

void AppenderFile::closeFile()
{
    if (m_logfile)
    {
        fclose(m_logfile);
        m_logfile = NULL;
    }
}
