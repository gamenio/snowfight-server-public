#include "LogOperation.h"
#include "Logger.h"

int LogOperation::call()
{
    m_logger->write(m_msg.get());
    return 0;
}
