#ifndef __APPENDERFILE_H__
#define __APPENDERFILE_H__

#include <atomic>
#include "Appender.h"

class AppenderFile : public Appender
{
    public:
        typedef std::integral_constant<AppenderType, APPENDER_FILE>::type TypeIndex;

        AppenderFile(uint8 id, std::string const& name, LogLevel level, AppenderFlags flags, ExtraAppenderArgs extraArgs);
        ~AppenderFile();
        FILE* openFile(std::string const& name, std::string const& mode, bool backup);
        AppenderType getType() const override { return TypeIndex::value; }

    private:
        void closeFile();
        void _write(LogMessage const* message) override;

        FILE* m_logfile;
        std::string m_fileName;
        std::string m_logDir;
        bool m_dynamicName;
        bool m_backup;
        uint64 m_maxFileSize;
		uint64 m_fileSize;
};

#endif //__APPENDERFILE_H__
