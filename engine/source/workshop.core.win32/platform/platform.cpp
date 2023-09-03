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

void message_dialog(const char* text, message_dialog_type type)
{
    std::string caption = "";
    UINT native_type = 0;

    switch (type)
    {
        case message_dialog_type::error:
        {
            caption = "Error";
            native_type = MB_ICONERROR | MB_OK;
            break;
        }
        case message_dialog_type::warning:
        {
            caption = "Warning";
            native_type = MB_ICONWARNING | MB_OK;
            break;
        }
        case message_dialog_type::message:
        {
            caption = "Message";
            native_type = MB_ICONINFORMATION | MB_OK;
            break;
        }
    }

    MessageBoxA(nullptr, text, caption.c_str(), native_type);
}

}; // namespace workshop
