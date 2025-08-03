#ifndef __APPENDERCONSOLE_H__
#define __APPENDERCONSOLE_H__

#include <string>
#include "Appender.h"

enum ColorTypes
{
    BLACK,
    RED,
    GREEN,
    BROWN,
    BLUE,
    MAGENTA,
    CYAN,
    GREY,
    YELLOW,
    LRED,
    LGREEN,
    LBLUE,
    LMAGENTA,
    LCYAN,
    WHITE
};

const uint8 MaxColors = uint8(WHITE) + 1;

class AppenderConsole : public Appender
{
    public:
        typedef std::integral_constant<AppenderType, APPENDER_CONSOLE>::type TypeIndex;

        AppenderConsole(uint8 id, std::string const& name, LogLevel level, AppenderFlags flags, ExtraAppenderArgs extraArgs);
        void initColors(const std::string& init_str);
        AppenderType getType() const override { return TypeIndex::value; }

    private:
        void setColor(bool stdout_stream, ColorTypes color);
        void resetColor(bool stdout_stream);
        void _write(LogMessage const* message) override;
        bool m_colored;
        ColorTypes m_colors[MaxLogLevels];
};

#endif //__APPENDERCONSOLE_H__
