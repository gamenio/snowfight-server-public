#include "Log.h"

#include <cstdio>
#include <sstream>

#include "configuration/Config.h"
#include "utilities/TimeUtil.h"
#include "utilities/StringUtil.h"
#include "AppenderConsole.h"
#include "AppenderFile.h"
#include "LogOperation.h"


Log::Log() : m_appenderId(0), m_lowestLogLevel(LOG_LEVEL_FATAL), m_ioService(nullptr), m_strand(nullptr)
{
    m_logsTimestamp = "_" + getTimestampStr();
    registerAppender<AppenderConsole>();
    registerAppender<AppenderFile>();
}

Log::~Log()
{
    delete m_strand;
    close();
}

uint8 Log::nextAppenderId()
{
    return m_appenderId++;
}

int32 GetConfigIntDefault(std::string base, const char* name, int32 value)
{
    base.append(name);
    return sConfigMgr->getIntDefault(base.c_str(), value);
}

std::string GetConfigStringDefault(std::string base, const char* name, const char* value)
{
    base.append(name);
    return sConfigMgr->getStringDefault(base.c_str(), value);
}

Appender* Log::getAppenderByName(std::string const& name)
{
    AppenderMap::iterator it = m_appenders.begin();
    while (it != m_appenders.end() && it->second && it->second->getName() != name)
        ++it;

    return it == m_appenders.end() ? NULL : it->second;
}

void Log::createAppenderFromConfig(std::string const& appenderName)
{
    if (appenderName.empty())
        return;

    // Format=type, level, flags, optional1, optional2
    // if type = File. optional1 = file and option2 = mode
    // if type = Console. optional1 = Color
    std::string options = sConfigMgr->getStringDefault(appenderName.c_str(), "");

    Tokenizer tokens(options, ',');
    Tokenizer::const_iterator iter = tokens.begin();

    size_t size = tokens.size();
    std::string name = appenderName.substr(9);

    if (size < 2)
    {
        fprintf(stderr, "Log::CreateAppenderFromConfig: Wrong configuration for appender %s. Config line: %s\n", name.c_str(), options.c_str());
        return;
    }

    AppenderFlags flags = APPENDER_FLAGS_NONE;
    AppenderType type = AppenderType(atoi(*iter++));
    LogLevel level = LogLevel(atoi(*iter++));

    if (level > LOG_LEVEL_FATAL)
    {
        fprintf(stderr, "Log::CreateAppenderFromConfig: Wrong Log Level %d for appender %s\n", level, name.c_str());
        return;
    }

    if (size > 2)
        flags = AppenderFlags(atoi(*iter++));

    auto factoryFunction = m_appenderFactory.find(type);
    if (factoryFunction == m_appenderFactory.end())
    {
        fprintf(stderr, "Log::CreateAppenderFromConfig: Unknown type %d for appender %s\n", type, name.c_str());
        return;
    }

    try
    {
        Appender* appender = factoryFunction->second(nextAppenderId(), name, level, flags, ExtraAppenderArgs(iter, tokens.end()));
        m_appenders[appender->getId()] = appender;
    }
    catch (InvalidAppenderArgsException const& iaae)
    {
        fprintf(stderr, "%s", iaae.what());
    }
}

void Log::createLoggerFromConfig(std::string const& loggerName)
{
    if (loggerName.empty())
        return;

    LogLevel level = LOG_LEVEL_DISABLED;
    uint8 type = uint8(-1);

    std::string options = sConfigMgr->getStringDefault(loggerName.c_str(), "");
    std::string name = loggerName.substr(7);

    if (options.empty())
    {
        fprintf(stderr, "Log::CreateLoggerFromConfig: Missing config option Logger.%s\n", name.c_str());
        return;
    }

    Tokenizer tokens(options, ',');
    Tokenizer::const_iterator iter = tokens.begin();

    if (tokens.size() != 2)
    {
        fprintf(stderr, "Log::CreateLoggerFromConfig: Wrong config option Logger.%s=%s\n", name.c_str(), options.c_str());
        return;
    }

    Logger& logger = m_loggers[name];
    if (!logger.getName().empty())
    {
        fprintf(stderr, "Error while configuring Logger %s. Already defined\n", name.c_str());
        return;
    }

    level = LogLevel(atoi(*iter++));
    if (level > LOG_LEVEL_FATAL)
    {
        fprintf(stderr, "Log::CreateLoggerFromConfig: Wrong Log Level %u for logger %s\n", type, name.c_str());
        return;
    }

    if (level < m_lowestLogLevel)
        m_lowestLogLevel = level;

    logger.create(name, level);
    //fprintf(stdout, "Log::CreateLoggerFromConfig: Created Logger %s, Level %u\n", name.c_str(), level);

    std::istringstream ss(*iter);
    std::string str;

    ss >> str;
    while (ss)
    {
        if (Appender* appender = getAppenderByName(str))
        {
            logger.addAppender(appender->getId(), appender);
            //fprintf(stdout, "Log::CreateLoggerFromConfig: Added Appender %s to Logger %s\n", appender->getName().c_str(), name.c_str());
        }
        else
            fprintf(stderr, "Error while configuring Appender %s in Logger %s. Appender does not exist", str.c_str(), name.c_str());
        ss >> str;
    }
}

void Log::readAppendersFromConfig()
{
    std::list<std::string> keys = sConfigMgr->getKeysByString("Appender.");

    while (!keys.empty())
    {
        createAppenderFromConfig(keys.front());
        keys.pop_front();
    }
}

void Log::readLoggersFromConfig()
{
    std::list<std::string> keys = sConfigMgr->getKeysByString("Logger.");

    while (!keys.empty())
    {
        createLoggerFromConfig(keys.front());
        keys.pop_front();
    }

    // Bad config configuration, creating default config
    if (m_loggers.find(LOGGER_ROOT) == m_loggers.end())
    {
        fprintf(stderr, "Wrong Loggers configuration. Review your Logger config section.\n"
                        "Creating default loggers [root (Error), server (Info)] to console\n");

        close(); // Clean any Logger or Appender created

        AppenderConsole* appender = new AppenderConsole(nextAppenderId(), "Console", LOG_LEVEL_DEBUG, APPENDER_FLAGS_NONE, ExtraAppenderArgs());
        m_appenders[appender->getId()] = appender;

        Logger& rootLogger = m_loggers[LOGGER_ROOT];
        rootLogger.create(LOGGER_ROOT, LOG_LEVEL_ERROR);
        rootLogger.addAppender(appender->getId(), appender);

        Logger& serverLogger = m_loggers["server"];
        serverLogger.create("server", LOG_LEVEL_INFO);
        serverLogger.addAppender(appender->getId(), appender);
    }
}

void Log::write(std::unique_ptr<LogMessage>&& msg) const
{
    Logger const* logger = getLoggerByType(msg->m_type);

    if (m_ioService)
    {
        auto logOperation = std::shared_ptr<LogOperation>(new LogOperation(logger, std::move(msg)));

        m_ioService->post(m_strand->wrap([logOperation](){ logOperation->call(); }));
    }
    else
        logger->write(msg.get());
}

std::string Log::getTimestampStr()
{
    time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::tm aTm;
    localtime_r(&tt, &aTm);

    //       YYYY   year
    //       MM     month (2 digits 01-12)
    //       DD     day (2 digits 01-31)
    //       HH     hour (2 digits 00-23)
    //       MM     minutes (2 digits 00-59)
    //       SS     seconds (2 digits 00-59)
    return StringUtil::format("%04d-%02d-%02d_%02d-%02d-%02d",
        aTm.tm_year + 1900, aTm.tm_mon + 1, aTm.tm_mday, aTm.tm_hour, aTm.tm_min, aTm.tm_sec);
}

bool Log::setLogLevel(std::string const& name, int32 level, bool isLogger /* = true */)
{
    LogLevel newLevel = LogLevel(level);
    if (newLevel < 0 || newLevel > MaxLogLevels)
        return false;

    if (isLogger)
    {
        LoggerMap::iterator it = m_loggers.begin();
        while (it != m_loggers.end() && it->second.getName() != name)
            ++it;

        if (it == m_loggers.end())
            return false;

        it->second.setLogLevel(newLevel);

        if (newLevel != LOG_LEVEL_DISABLED && newLevel < m_lowestLogLevel)
            m_lowestLogLevel = newLevel;
    }
    else
    {
        Appender* appender = getAppenderByName(name);
        if (!appender)
            return false;

        appender->setLogLevel(newLevel);
    }

    return true;
}

void Log::outCharDump(char const* str, uint32 accountId, uint32 guid, char const* name)
{
    if (!str || !shouldLog("entities.player.dump", LOG_LEVEL_INFO))
        return;

    std::ostringstream ss;
    ss << "== START DUMP == (account: " << accountId << " guid: " << guid << " name: " << name
       << ")\n" << str << "\n== END DUMP ==\n";

    std::unique_ptr<LogMessage> msg(new LogMessage(LOG_LEVEL_INFO, "entities.player.dump", ss.str()));
    std::ostringstream param;
    param << guid << '_' << name;

    msg->m_param1 = param.str();

    write(std::move(msg));
}

void Log::setRealmId(uint32 id)
{
    for (AppenderMap::iterator it = m_appenders.begin(); it != m_appenders.end(); ++it)
        it->second->setRealmId(id);
}

void Log::close()
{
    m_loggers.clear();
    for (AppenderMap::iterator it = m_appenders.begin(); it != m_appenders.end(); ++it)
        delete it->second;

    m_appenders.clear();
}

Log* Log::instance()
{
    static Log instance;
    return &instance;
}

void Log::initialize(boost::asio::io_service* ioService, std::string const& defLogsDir)
{
    if (ioService)
    {
        m_ioService = ioService;
        m_strand = new boost::asio::io_service::strand(*ioService);
    }
	m_logsDir = defLogsDir;
    loadFromConfig();
}

void Log::setSynchronous()
{
    delete m_strand;
    m_strand = nullptr;
    m_ioService = nullptr;
}

void Log::loadFromConfig()
{
    close();

    m_lowestLogLevel = LOG_LEVEL_FATAL;
    m_appenderId = 0;
    std::string logsDir = sConfigMgr->getStringDefault("LogsDir", "");
	if (!logsDir.empty())
		m_logsDir = logsDir;

    if (!m_logsDir.empty())
        if ((m_logsDir.at(m_logsDir.length() - 1) != '/') && (m_logsDir.at(m_logsDir.length() - 1) != '\\'))
            m_logsDir.push_back('/');

    readAppendersFromConfig();
    readLoggersFromConfig();
}
