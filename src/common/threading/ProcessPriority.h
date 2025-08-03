#ifndef __PROCESSPRIO_H__
#define __PROCESSPRIO_H__

#include "logging/Log.h"
#include "configuration/Config.h"

#if PLATFORM == PLATFORM_UNIX
#include <sched.h>
#include <sys/resource.h>
#define PROCESS_HIGH_PRIORITY -15 // [-20, 19], default is 0
#endif

void setProcessPriority(const std::string& logChannel)
{
// Suppresses Mac OS X Warning since logChannel isn't used.
#if PLATFORM == PLATFORM_APPLE
    (void)logChannel;
#endif

#if (PLATFORM == PLATFORM_WINDOWS) || (PLATFORM == PLATFORM_UNIX)
    ///- Handle affinity for multiple processors and process priority
    uint32 affinity = sConfigMgr->getIntDefault("UseProcessors", 0);
    bool highPriority = sConfigMgr->getBoolDefault("ProcessPriority", false);

#if PLATFORM == PLATFORM_WINDOWS

    HANDLE hProcess = GetCurrentProcess();
    if (affinity > 0)
    {
        ULONG_PTR appAff;
        ULONG_PTR sysAff;

        if (GetProcessAffinityMask(hProcess, &appAff, &sysAff))
        {
            // remove non accessible processors
            ULONG_PTR currentAffinity = affinity & appAff;

            if (!currentAffinity)
                NS_LOG_ERROR(logChannel, "Processors marked in UseProcessors bitmask (hex) %x are not accessible. Accessible processors bitmask (hex): %x", affinity, appAff);
            else if (SetProcessAffinityMask(hProcess, currentAffinity))
				NS_LOG_INFO(logChannel, "Using processors (bitmask, hex): %x", currentAffinity);
            else
				NS_LOG_ERROR(logChannel, "Can't set used processors (hex): %x", currentAffinity);
        }
    }

    if (highPriority)
    {
        if (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
			NS_LOG_INFO(logChannel, "Process priority class set to HIGH");
        else
			NS_LOG_ERROR(logChannel, "Can't set process priority class.");
    }

#else // PLATFORM_UNIX

    if (affinity > 0)
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);

        for (unsigned int i = 0; i < sizeof(affinity) * 8; ++i)
            if (affinity & (1 << i))
                CPU_SET(i, &mask);

        if (sched_setaffinity(0, sizeof(mask), &mask))
			NS_LOG_ERROR(logChannel, "Can't set used processors (hex): %x, error: %s", affinity, strerror(errno));
        else
        {
            CPU_ZERO(&mask);
            sched_getaffinity(0, sizeof(mask), &mask);
			NS_LOG_INFO(logChannel, "Using processors (bitmask, hex): %lx", *(__cpu_mask*)(&mask));
        }
    }

    if (highPriority)
    {
        if (setpriority(PRIO_PROCESS, 0, PROCESS_HIGH_PRIORITY))
			NS_LOG_ERROR(logChannel, "Can't set process priority class, error: %s", strerror(errno));
        else
			NS_LOG_INFO(logChannel, "Process priority class set to %i", getpriority(PRIO_PROCESS, 0));
    }

#endif
#endif
}

#endif //__PROCESSPRIO_H__
