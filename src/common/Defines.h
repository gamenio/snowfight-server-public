#ifndef __DEFINES_H__
#define __DEFINES_H__

#include "CompilerDefs.h"

#include <cinttypes>
#include <climits>

#if PLATFORM == PLATFORM_WINDOWS
	#define NS_PATH_MAX MAX_PATH
	#ifndef DECLSPEC_NORETURN
		#define DECLSPEC_NORETURN __declspec(noreturn)
	#endif //DECLSPEC_NORETURN
	#ifndef DECLSPEC_DEPRECATED
		#define DECLSPEC_DEPRECATED __declspec(deprecated)
	#endif //DECLSPEC_DEPRECATED
#else //PLATFORM != PLATFORM_WINDOWS
	#define NS_PATH_MAX PATH_MAX
	#define DECLSPEC_NORETURN
	#define DECLSPEC_DEPRECATED
#endif //PLATFORM


#if COMPILER == COMPILER_GNU
	#define ATTR_NORETURN __attribute__((__noreturn__))
	#define ATTR_PRINTF(F, V) __attribute__ ((__format__ (__printf__, F, V)))
	#define ATTR_DEPRECATED __attribute__((__deprecated__))
#else //COMPILER != COMPILER_GNU
	#define ATTR_NORETURN
	#define ATTR_PRINTF(F, V)
	#define ATTR_DEPRECATED
#endif //COMPILER == COMPILER_GNU

#define UI64FMTD "%" PRIu64
#define UI64LIT(N) UINT64_C(N)

#define SI64FMTD "%" PRId64
#define SI64LIT(N) INT64_C(N)

#define SZFMTD "%" PRIuPTR

// Basic data type definitions
typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;


typedef int32 NSTime;




#endif //__DEFINES_H__
