#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "Appender.h"

class Logger
{
    public:
        Logger();

        void create(std::string const& name, LogLevel level);
        void addAppender(uint8 type, Appender *);
        void delAppender(uint8 type);

        std::string const& getName() const;
        LogLevel getLogLevel() const;
        void setLogLevel(LogLevel level);
        void write(LogMessage* message) const;

    private:
        std::string m_name;
        LogLevel m_level;
        AppenderMap m_appenders;
};

#endif //__LOGGER_H__
