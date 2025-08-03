#include "Appender.h"

#include <utility>
#include <sstream>
#include <iomanip>

#include "Common.h"
#include "utilities/TimeUtil.h"
#include "utilities/StringUtil.h"

Appender::Appender(uint8 id, std::string const& _name, LogLevel _level /* = LOG_LEVEL_DISABLED */, AppenderFlags _flags /* = APPENDER_FLAGS_NONE */):
m_id(id), m_name(_name), m_level(_level), m_flags(_flags) { }

Appender::~Appender() { }

uint8 Appender::getId() const
{
    return m_id;
}

std::string const& Appender::getName() const
{
    return m_name;
}

LogLevel Appender::getLogLevel() const
{
    return m_level;
}

AppenderFlags Appender::getFlags() const
{
    return m_flags;
}

void Appender::setLogLevel(LogLevel _level)
{
    m_level = _level;
}

void Appender::write(LogMessage* message)
{
    if (!m_level || m_level > message->m_level)
        return;

    std::ostringstream ss;

    if (m_flags & APPENDER_FLAGS_PREFIX_TIMESTAMP)
        ss << getNowTimeStr() << ' ';

    if (m_flags & APPENDER_FLAGS_PREFIX_LOGLEVEL)
        ss << StringUtil::format("%-5s ", Appender::getLogLevelString(message->m_level));

    if (m_flags & APPENDER_FLAGS_PREFIX_LOGFILTERTYPE)
        ss << '[' << message->m_type << "] ";

    message->m_prefix = ss.str();

	std::lock_guard<std::mutex> lock(m_mutex);
    _write(message);
}

const char* Appender::getLogLevelString(LogLevel level)
{
    switch (level)
    {
        case LOG_LEVEL_FATAL:
            return "FATAL";
        case LOG_LEVEL_ERROR:
            return "ERROR";
        case LOG_LEVEL_WARN:
            return "WARN";
        case LOG_LEVEL_INFO:
            return "INFO";
        case LOG_LEVEL_DEBUG:
            return "DEBUG";
        case LOG_LEVEL_TRACE:
            return "TRACE";
        default:
            return "DISABLED";
    }
}
