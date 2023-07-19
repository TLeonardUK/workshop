// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/platform/platform.h"

#include <Windows.h>
#include <psapi.h>

namespace ws {

platform_type get_platform()
{
    return platform_type::windows;
}

config_type get_config()
{
#if defined(WS_DEBUG)
    return config_type::debug;
#elif defined(WS_PROFILE)
    return config_type::profile;
#elif defined(WS_RELEASE)
    return config_type::release;
#else
    #error Unknown configuration mode
#endif
}

size_t get_memory_usage()
{
    PROCESS_MEMORY_COUNTERS_EX counters;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&counters, sizeof(counters));
    return counters.PrivateUsage;
}

size_t get_pagefile_usage()
{
    PROCESS_MEMORY_COUNTERS_EX counters;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&counters, sizeof(counters));
    return counters.QuotaPagedPoolUsage;
}

}; // namespace workshop
