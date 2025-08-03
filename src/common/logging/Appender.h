#ifndef __APPENDER_H__
#define __APPENDER_H__

#include <stdexcept>
#include <time.h>
#include <type_traits>
#include <utility>
#include <mutex>

#include "Common.h"

enum LogLevel
{
    LOG_LEVEL_DISABLED                           = 0,
    LOG_LEVEL_TRACE                              = 1,
    LOG_LEVEL_DEBUG                              = 2,
    LOG_LEVEL_INFO                               = 3,
    LOG_LEVEL_WARN                               = 4,
    LOG_LEVEL_ERROR                              = 5,
    LOG_LEVEL_FATAL                              = 6
};

const uint8 MaxLogLevels = 6;

enum AppenderType : uint8
{
    APPENDER_NONE,
    APPENDER_CONSOLE,
    APPENDER_FILE,
    APPENDER_DB
};

enum AppenderFlags
{
    APPENDER_FLAGS_NONE                          = 0,
    APPENDER_FLAGS_PREFIX_TIMESTAMP              = 1 << 0,
    APPENDER_FLAGS_PREFIX_LOGLEVEL               = 1 << 1,
    APPENDER_FLAGS_PREFIX_LOGFILTERTYPE          = 1 << 2,
    APPENDER_FLAGS_USE_TIMESTAMP                 = 1 << 3, // only used by FileAppender
    APPENDER_FLAGS_MAKE_FILE_BACKUP              = 1 << 4  // only used by FileAppender
};

struct LogMessage
{
    LogMessage(LogLevel level, std::string const& type, std::string&& text)
        : m_level(level), m_type(type), m_text(std::forward<std::string>(text))
    { }

    LogMessage(LogMessage const& /*other*/) = delete;
    LogMessage& operator=(LogMessage const& /*other*/) = delete;

    LogLevel const m_level;
    std::string const m_type;
    std::string const m_text;
    std::string m_prefix;
    std::string m_param1;

    ///@ Returns size of the log message content in bytes
    uint32 size() const
    {
        return static_cast<uint32>(m_prefix.size() + m_text.size());
    }
};

class Appender
{
    public:
        Appender(uint8 id, std::string const& name, LogLevel level = LOG_LEVEL_DISABLED, AppenderFlags flags = APPENDER_FLAGS_NONE);
        virtual ~Appender();

        uint8 getId() const;
        std::string const& getName() const;
        virtual AppenderType getType() const = 0;
        LogLevel getLogLevel() const;
        AppenderFlags getFlags() const;

        void setLogLevel(LogLevel);
        void write(LogMessage* message);
        static const char* getLogLevelString(LogLevel level);
        virtual void setRealmId(uint32 /*realmId*/) { }

    private:
        virtual void _write(LogMessage const* /*message*/) = 0;

        uint8 m_id;
        std::string m_name;
        LogLevel m_level;
        AppenderFlags m_flags;
		std::mutex m_mutex;
};

typedef std::unordered_map<uint8, Appender*> AppenderMap;

typedef std::vector<char const*> ExtraAppenderArgs;
typedef Appender*(*AppenderCreatorFn)(uint8 id, std::string const& name, LogLevel level, AppenderFlags flags, ExtraAppenderArgs extraArgs);
typedef std::unordered_map<uint8, AppenderCreatorFn> AppenderCreatorMap;

template<class AppenderImpl>
Appender* createAppender(uint8 id, std::string const& name, LogLevel level, AppenderFlags flags, ExtraAppenderArgs extraArgs)
{
    return new AppenderImpl(id, name, level, flags, std::forward<ExtraAppenderArgs>(extraArgs));
}

class InvalidAppenderArgsException : public std::length_error
{
public:
    explicit InvalidAppenderArgsException(std::string const& message) : std::length_error(message) { }
};

#endif //__APPENDER_H__
