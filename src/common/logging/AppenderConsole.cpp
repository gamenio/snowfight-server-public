#include <sstream>

#include "configuration/Config.h"
#include "utilities/Util.h"
#include "AppenderConsole.h"

#if PLATFORM == PLATFORM_WINDOWS
  #include <Windows.h>
#endif

AppenderConsole::AppenderConsole(uint8 id, std::string const& name, LogLevel level, AppenderFlags flags, ExtraAppenderArgs extraArgs)
    : Appender(id, name, level, flags), m_colored(false)
{
    for (uint8 i = 0; i < MaxLogLevels; ++i)
        m_colors[i] = ColorTypes(MaxColors);

    if (!extraArgs.empty())
        initColors(extraArgs[0]);
}

void AppenderConsole::initColors(std::string const& str)
{
    if (str.empty())
    {
        m_colored = false;
        return;
    }

    int color[MaxLogLevels];

    std::istringstream ss(str);

    for (uint8 i = 0; i < MaxLogLevels; ++i)
    {
        ss >> color[i];

        if (!ss)
            return;

        if (color[i] < 0 || color[i] >= MaxColors)
            return;
    }

    for (uint8 i = 0; i < MaxLogLevels; ++i)
        m_colors[i] = ColorTypes(color[i]);

    m_colored = true;
}

void AppenderConsole::setColor(bool stdout_stream, ColorTypes color)
{
#if PLATFORM == PLATFORM_WINDOWS
    static WORD WinColorFG[MaxColors] =
    {
        0,                                                  // BLACK
        FOREGROUND_RED,                                     // RED
        FOREGROUND_GREEN,                                   // GREEN
        FOREGROUND_RED | FOREGROUND_GREEN,                  // BROWN
        FOREGROUND_BLUE,                                    // BLUE
        FOREGROUND_RED |                    FOREGROUND_BLUE, // MAGENTA
        FOREGROUND_GREEN | FOREGROUND_BLUE,                 // CYAN
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, // WHITE
                                                            // YELLOW
        FOREGROUND_RED | FOREGROUND_GREEN |                   FOREGROUND_INTENSITY,
                                                            // RED_BOLD
        FOREGROUND_RED |                                      FOREGROUND_INTENSITY,
                                                            // GREEN_BOLD
        FOREGROUND_GREEN |                   FOREGROUND_INTENSITY,
        FOREGROUND_BLUE | FOREGROUND_INTENSITY,             // BLUE_BOLD
                                                            // MAGENTA_BOLD
        FOREGROUND_RED |                    FOREGROUND_BLUE | FOREGROUND_INTENSITY,
                                                            // CYAN_BOLD
        FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
                                                            // WHITE_BOLD
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
    };

    HANDLE hConsole = GetStdHandle(stdout_stream ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
    SetConsoleTextAttribute(hConsole, WinColorFG[color]);
#else
    enum ANSITextAttr
    {
        TA_NORMAL                                = 0,
        TA_BOLD                                  = 1,
        TA_BLINK                                 = 5,
        TA_REVERSE                               = 7
    };

    enum ANSIFgTextAttr
    {
        FG_BLACK                                 = 30,
        FG_RED,
        FG_GREEN,
        FG_BROWN,
        FG_BLUE,
        FG_MAGENTA,
        FG_CYAN,
        FG_WHITE,
        FG_YELLOW
    };

    enum ANSIBgTextAttr
    {
        BG_BLACK                                 = 40,
        BG_RED,
        BG_GREEN,
        BG_BROWN,
        BG_BLUE,
        BG_MAGENTA,
        BG_CYAN,
        BG_WHITE
    };

    static uint8 UnixColorFG[MaxColors] =
    {
        FG_BLACK,                                          // BLACK
        FG_RED,                                            // RED
        FG_GREEN,                                          // GREEN
        FG_BROWN,                                          // BROWN
        FG_BLUE,                                           // BLUE
        FG_MAGENTA,                                        // MAGENTA
        FG_CYAN,                                           // CYAN
        FG_WHITE,                                          // WHITE
        FG_YELLOW,                                         // YELLOW
        FG_RED,                                            // LRED
        FG_GREEN,                                          // LGREEN
        FG_BLUE,                                           // LBLUE
        FG_MAGENTA,                                        // LMAGENTA
        FG_CYAN,                                           // LCYAN
        FG_WHITE                                           // LWHITE
    };

    fprintf((stdout_stream? stdout : stderr), "\x1b[%d%sm", UnixColorFG[color], (color >= YELLOW && color < MaxColors ? ";1" : ""));
    #endif
}

void AppenderConsole::resetColor(bool stdout_stream)
{
    #if PLATFORM == PLATFORM_WINDOWS
    HANDLE hConsole = GetStdHandle(stdout_stream ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
    #else
    fprintf((stdout_stream ? stdout : stderr), "\x1b[0m");
    #endif
}

void AppenderConsole::_write(LogMessage const* message)
{
    bool stdout_stream = !(message->m_level == LOG_LEVEL_ERROR || message->m_level == LOG_LEVEL_FATAL);

    if (m_colored)
    {
        uint8 index;
        switch (message->m_level)
        {
            case LOG_LEVEL_TRACE:
               index = 5;
               break;
            case LOG_LEVEL_DEBUG:
               index = 4;
               break;
            case LOG_LEVEL_INFO:
               index = 3;
               break;
            case LOG_LEVEL_WARN:
               index = 2;
               break;
            case LOG_LEVEL_FATAL:
               index = 0;
               break;
            case LOG_LEVEL_ERROR: // No break on purpose
            default:
               index = 1;
               break;
        }

        setColor(stdout_stream, m_colors[index]);
        utf8printf(stdout_stream ? stdout : stderr, "%s%s\n", message->m_prefix.c_str(), message->m_text.c_str());
        resetColor(stdout_stream);
    }
    else
        utf8printf(stdout_stream ? stdout : stderr, "%s%s\n", message->m_prefix.c_str(), message->m_text.c_str());
}
