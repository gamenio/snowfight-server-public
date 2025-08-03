#include "Errors.h"

#include <cstdio>
#include <cstdlib>
#include <thread>
#include <cstdarg>

/**
    @file Errors.cpp

    @brief This file contains definitions of functions used for reporting critical application errors

    It is very important that (std::)abort is NEVER called in place of *((volatile int*)NULL) = 0;
    Calling abort() on Windows does not invoke unhandled exception filters - a mechanism used by WheatyExceptionReport
    to log crashes. exit(1) calls here are for static analysis tools to indicate that calling functions defined in this file
    terminates the application.
 */

namespace errors {

void _assert(char const* file, int line, char const* function, char const* message)
{
    fprintf(stderr, "\n%s:%i in %s ASSERTION FAILED:\n  %s\n",
            file, line, function, message);
    *((volatile int*)NULL) = 0;
    exit(1);
}

void _assert(char const* file, int line, char const* function, char const* message, char const* format, ...)
{
    va_list args;
    va_start(args, format);

    fprintf(stderr, "\n%s:%i in %s ASSERTION FAILED:\n  %s ", file, line, function, message);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    fflush(stderr);

    va_end(args);
    *((volatile int*)NULL) = 0;
    exit(1);
}

void _fatal(char const* file, int line, char const* function, char const* message, ...)
{
    va_list args;
    va_start(args, message);

    fprintf(stderr, "\n%s:%i in %s FATAL ERROR:\n  ", file, line, function);
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    fflush(stderr);

	va_end(args);
    *((volatile int*)NULL) = 0;
    exit(1);
}

void _error(char const* file, int line, char const* function, char const* message)
{
    fprintf(stderr, "\n%s:%i in %s ERROR:\n  %s\n",
                   file, line, function, message);
    *((volatile int*)NULL) = 0;
    exit(1);
}

void _warning(char const* file, int line, char const* function, char const* message)
{
    fprintf(stderr, "\n%s:%i in %s WARNING:\n  %s\n",
                   file, line, function, message);
}

void _abort(char const* file, int line, char const* function)
{
    fprintf(stderr, "\n%s:%i in %s ABORTED\n",
                   file, line, function);
    *((volatile int*)NULL) = 0;
    exit(1);
}

void abortHandler(int /*sigval*/)
{
    // nothing useful to log here, no way to pass args
    *((volatile int*)NULL) = 0;
    exit(1);
}

} // namespace errors
