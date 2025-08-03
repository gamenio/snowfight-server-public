#ifndef __ERRORS_H__
#define __ERRORS_H__

#include "Common.h"


namespace errors
{
    DECLSPEC_NORETURN  void _assert(char const* file, int line, char const* function, char const* message) ATTR_NORETURN;
    DECLSPEC_NORETURN  void _assert(char const* file, int line, char const* function, char const* message, char const* format, ...) ATTR_NORETURN ATTR_PRINTF(5, 6);

    DECLSPEC_NORETURN  void _fatal(char const* file, int line, char const* function, char const* message, ...) ATTR_NORETURN ATTR_PRINTF(4, 5);

    DECLSPEC_NORETURN  void _error(char const* file, int line, char const* function, char const* message) ATTR_NORETURN;

    DECLSPEC_NORETURN  void _abort(char const* file, int line, char const* function) ATTR_NORETURN;

    void _warning(char const* file, int line, char const* function, char const* message);

    DECLSPEC_NORETURN  void abortHandler(int sigval) ATTR_NORETURN;

} // namespace errors



#if COMPILER == COMPILER_MICROSOFT
#define ASSERT_BEGIN __pragma(warning(push)) __pragma(warning(disable: 4127))
#define ASSERT_END __pragma(warning(pop))
#else
#define ASSERT_BEGIN
#define ASSERT_END
#endif

#if NS_DEBUG
#define NS_ASSERT_DEBUG(cond, ...) NS_ASSERT(cond, __VA_ARGS__)
#else
#define NS_ASSERT_DEBUG(cond, ...)
#endif

#define NS_ASSERT(cond, ...) ASSERT_BEGIN do { if (!(cond)) errors::_assert(__FILE__, __LINE__, __FUNCTION__, #cond, ##__VA_ARGS__); } while(0) ASSERT_END
#define NS_ASSERT_FATAL(cond, ...) ASSERT_BEGIN do { if (!(cond)) errors::_fatal(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); } while(0) ASSERT_END
#define NS_ASSERT_ERROR(cond, msg) ASSERT_BEGIN do { if (!(cond)) errors::_error(__FILE__, __LINE__, __FUNCTION__, (msg)); } while(0) ASSERT_END
#define NS_ASSERT_WARNING(cond, msg) ASSERT_BEGIN do { if (!(cond)) errors::_warning(__FILE__, __LINE__, __FUNCTION__, (msg)); } while(0) ASSERT_END
#define NS_ASSERT_ABORT() ASSERT_BEGIN do { errors::_abort(__FILE__, __LINE__, __FUNCTION__); } while(0) ASSERT_END


template <typename T> inline T* assertNotNull(T* pointer)
{
    NS_ASSERT(pointer);
    return pointer;
}

#endif //__ERRORS_H__
