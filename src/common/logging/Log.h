#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>

#include "Common.h"
#include "utilities/StringUtil.h"
#include "Appender.h"
#include "Logger.h"



#define LOGGER_ROOT "root"

class Log
{
    typedef std::unordered_map<std::string, Logger> LoggerMap;

    private:
        Log();
        ~Log();

    public:

        static Log* instance();

        void initialize(boost::asio::io_service* ioService, std::string const& defLogsDir);
        void setSynchronous();  // Not threadsafe - should only be called from main() after all threads are joined
        void loadFromConfig();
        void close();
        bool shouldLog(std::string const& type, LogLevel level) const;
        bool setLogLevel(std::string const& name, int32 level, bool isLogger = true);

        template<typename Format, typename... Args>
        inline void outMessage(std::string const& filter, LogLevel const level, Format&& fmt, Args&&... args)
        {
			try
			{
				write(std_extensions::make_unique<LogMessage>(level, filter,
					StringUtil::format(std::forward<Format>(fmt), std::forward<Args>(args)...)));
			}
			catch (std::exception& e)
			{
				std::unique_ptr<LogMessage> msg = std_extensions::make_unique<LogMessage>(LOG_LEVEL_ERROR, "server",
					StringUtil::format("Wrong format occurred (%s) at %s:%u.", e.what(), __FILE__, __LINE__));
				write(std::move(msg));
			}

        }

        template<typename Format, typename... Args>
        void outCommand(uint32 account, Format&& fmt, Args&&... args)
        {
            if (!shouldLog("commands.gm", LOG_LEVEL_INFO))
                return;

            std::unique_ptr<LogMessage> msg =
				std_extensions::make_unique<LogMessage>(LOG_LEVEL_INFO, "commands.gm",
					StringUtil::format(std::forward<Format>(fmt), std::forward<Args>(args)...));

            msg->m_param1 = std::to_string(account);

            write(std::move(msg));
        }

        void outCharDump(char const* str, uint32 account_id, uint32 guid, char const* name);

        void setRealmId(uint32 id);

        template<class AppenderImpl>
        void registerAppender()
        {
            using Index = typename AppenderImpl::TypeIndex;
            auto itr = m_appenderFactory.find(Index::value);
            NS_ASSERT(itr == m_appenderFactory.end());
            m_appenderFactory[Index::value] = &createAppender<AppenderImpl>;
        }

        std::string const& getLogsDir() const { return m_logsDir; }
        std::string const& getLogsTimestamp() const { return m_logsTimestamp; }
		static std::string getTimestampStr();
    private:
        void write(std::unique_ptr<LogMessage>&& msg) const;

        Logger const* getLoggerByType(std::string const& type) const;
        Appender* getAppenderByName(std::string const& name);
        uint8 nextAppenderId();
        void createAppenderFromConfig(std::string const& name);
        void createLoggerFromConfig(std::string const& name);
        void readAppendersFromConfig();
        void readLoggersFromConfig();

        AppenderCreatorMap m_appenderFactory;
        AppenderMap m_appenders;
        LoggerMap m_loggers;
        uint8 m_appenderId;
        LogLevel m_lowestLogLevel;

        std::string m_logsDir;
        std::string m_logsTimestamp;

        boost::asio::io_service* m_ioService;
        boost::asio::io_service::strand* m_strand;
};

inline Logger const* Log::getLoggerByType(std::string const& type) const
{
    LoggerMap::const_iterator it = m_loggers.find(type);
    if (it != m_loggers.end())
        return &(it->second);

    if (type == LOGGER_ROOT)
        return NULL;

    std::string parentLogger = LOGGER_ROOT;
    size_t found = type.find_last_of(".");
    if (found != std::string::npos)
        parentLogger = type.substr(0,found);

    return getLoggerByType(parentLogger);
}

inline bool Log::shouldLog(std::string const& type, LogLevel level) const
{
    // TODO: Use cache to store "Type.sub1.sub2": "Type" equivalence, should
    // Speed up in cases where requesting "Type.sub1.sub2" but only configured
    // Logger "Type"

    // Don't even look for a logger if the LogLevel is lower than lowest log levels across all loggers
    if (level < m_lowestLogLevel)
        return false;

    Logger const* logger = getLoggerByType(type);
    if (!logger)
        return false;

    LogLevel logLevel = logger->getLogLevel();
	return logLevel != LOG_LEVEL_DISABLED && logLevel <= level;
}

#define sLog Log::instance()

#if PLATFORM != PLATFORM_WINDOWS
void check_args(const char*, ...) ATTR_PRINTF(1, 2);
void check_args(std::string const&, ...);

// This will catch format errors on build time
#define _LOG_MESSAGE_BODY(filterType__, level__, ...)                 \
        do {                                                            \
            if (sLog->shouldLog(filterType__, level__))                 \
            {                                                           \
                if (false)                                              \
                    check_args(__VA_ARGS__);                            \
                                                                        \
                sLog->outMessage(filterType__, level__, __VA_ARGS__);   \
            }                                                           \
        } while (0)
#else
#define _LOG_MESSAGE_BODY(filterType__, level__, ...)                 \
        __pragma(warning(push))                                         \
        __pragma(warning(disable:4127))                                 \
        do {                                                            \
            if (sLog->shouldLog(filterType__, level__))                 \
                sLog->outMessage(filterType__, level__, __VA_ARGS__);   \
        } while (0)                                                     \
        __pragma(warning(pop))
#endif

#define NS_LOG_TRACE(filterType__, ...) \
    _LOG_MESSAGE_BODY(filterType__, LOG_LEVEL_TRACE, __VA_ARGS__)

#define NS_LOG_DEBUG(filterType__, ...) \
    _LOG_MESSAGE_BODY(filterType__, LOG_LEVEL_DEBUG, __VA_ARGS__)

#define NS_LOG_INFO(filterType__, ...)  \
    _LOG_MESSAGE_BODY(filterType__, LOG_LEVEL_INFO, __VA_ARGS__)

#define NS_LOG_WARN(filterType__, ...)  \
    _LOG_MESSAGE_BODY(filterType__, LOG_LEVEL_WARN, __VA_ARGS__)

#define NS_LOG_ERROR(filterType__, ...) \
    _LOG_MESSAGE_BODY(filterType__, LOG_LEVEL_ERROR, __VA_ARGS__)

#define NS_LOG_FATAL(filterType__, ...) \
    _LOG_MESSAGE_BODY(filterType__, LOG_LEVEL_FATAL, __VA_ARGS__)

#endif //__LOG_H__
