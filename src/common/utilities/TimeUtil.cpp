#include "TimeUtil.h"

using namespace std::chrono;

#if PLATFORM == PLATFORM_WINDOWS
struct tm* localtime_r(const time_t* time, struct tm* result)
{
	localtime_s(result, time);
	return result;
}

#endif

std::string getDateTimeStr(system_clock::time_point const& timePoint)
{
	auto tpSec = time_point_cast<seconds>(timePoint);
	auto durMs = duration_cast<milliseconds>(timePoint - tpSec);
	auto tt = system_clock::to_time_t(timePoint);

	tm aTm;
	localtime_r(&tt, &aTm);
	char buff[25];
	snprintf(buff, sizeof(buff),
		"%04d/%02d/%02d %02d:%02d:%02d.%03d",
		aTm.tm_year + 1900,
		aTm.tm_mon + 1,
		aTm.tm_mday,
		aTm.tm_hour,
		aTm.tm_min,
		aTm.tm_sec,
		(int)durMs.count());

	return buff;
}

