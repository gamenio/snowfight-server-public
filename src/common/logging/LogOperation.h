#ifndef __LOGOPERATION_H__
#define __LOGOPERATION_H__

#include "Common.h"

class Logger;
struct LogMessage;

class LogOperation
{
    public:
        LogOperation(Logger const* logger, std::unique_ptr<LogMessage>&& msg)
            : m_logger(logger), m_msg(std::forward<std::unique_ptr<LogMessage>>(msg))
        { }

        ~LogOperation() { }

        int call();

    protected:
        Logger const* m_logger;
        std::unique_ptr<LogMessage> m_msg;
};

#endif //__LOGOPERATION_H__
