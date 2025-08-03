#ifndef __TIME_UTIL_H__
#define __TIME_UTIL_H__

#include <chrono>
#include <iomanip>

#include "Common.h"
#include "utilities/Util.h"

static std::chrono::system_clock::time_point sApplicationStartTime = std::chrono::system_clock::now();

inline int64 getUptimeMillis()
{
	using namespace std::chrono;

	int64 time = int64(duration_cast<milliseconds>(system_clock::now() - sApplicationStartTime).count());
	return time;
}

inline int64 getSystemTimeMillis()
{
	using namespace std::chrono;

	int64 time = int64(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
	return time;
}

inline double getHighResolutionTimeMillis()
{
	using namespace std::chrono;

	double msDouble = duration_cast<duration<double, std::milli>>(high_resolution_clock::now().time_since_epoch()).count();
	return msDouble;
}

inline NSTime convertSecToMillis(float seconds)
{
	return static_cast<NSTime>(seconds * 1000);
}

struct tm* localtime_r(const time_t* time, struct tm *result);
std::string getDateTimeStr(std::chrono::system_clock::time_point const& timePoint);
inline std::string getNowTimeStr()
{
	return getDateTimeStr(std::chrono::system_clock::now());
}


#endif //__TIME_UTIL_H__



