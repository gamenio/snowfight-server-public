#include "Logger.h"

Logger::Logger(): m_name(""), m_level(LOG_LEVEL_DISABLED) { }

void Logger::create(std::string const& _name, LogLevel _level)
{
    m_name = _name;
    m_level = _level;
}

std::string const& Logger::getName() const
{
    return m_name;
}

LogLevel Logger::getLogLevel() const
{
    return m_level;
}

void Logger::addAppender(uint8 id, Appender* appender)
{
    m_appenders[id] = appender;
}

void Logger::delAppender(uint8 id)
{
    m_appenders.erase(id);
}

void Logger::setLogLevel(LogLevel _level)
{
    m_level = _level;
}

void Logger::write(LogMessage* message) const
{
    if (!m_level || m_level > message->m_level || message->m_text.empty())
    {
        //fprintf(stderr, "Logger::write: Logger %s, Level %u. Msg %s Level %u WRONG LEVEL MASK OR EMPTY MSG\n", getName().c_str(), getLogLevel(), message.text.c_str(), message.level);
        return;
    }

    for (AppenderMap::const_iterator it = m_appenders.begin(); it != m_appenders.end(); ++it)
        if (it->second)
            it->second->write(message);
}
