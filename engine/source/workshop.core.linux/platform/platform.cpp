// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/platform/platform.h"

#include "thirdparty/nativefiledialog/src/include/nfd.h"

namespace ws {

platform_type get_platform()
{
    return platform_type::linux;
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
    // linux-todo
    return 0;
}

size_t get_total_memory()
{
    // linux-todo
    return 0;
}

size_t get_pagefile_usage()
{
    // linux-todo
    return 0;
}

void message_dialog(const char* text, message_dialog_type type)
{
    // linux-todo
}

std::string open_file_dialog(const char* text, const std::vector<file_dialog_filter>& filters)
{
    // linux-todo
    return "";
}

std::string save_file_dialog(const char* text, const std::vector<file_dialog_filter>& filters)
{
    // linux-todo
    return "";
}

std::string get_username()
{
    // linux-todo
    return "";
}

}; // namespace workshop
