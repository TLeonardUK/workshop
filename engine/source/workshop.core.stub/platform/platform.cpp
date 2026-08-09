// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/platform/platform.h"
#include "workshop.core/debug/debug.h"

namespace ws {

platform_type get_platform()
{
    return platform_type::stub;
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
    return 0;
}

size_t get_total_memory()
{
    return 0;
}

size_t get_pagefile_usage()
{
    return 0;
}

void message_dialog(const char* text, message_dialog_type type)
{
    db_log(core, "%s", text);
}

std::string open_file_dialog(const char* text, const std::vector<file_dialog_filter>& filters)
{
    return "";
}

std::string save_file_dialog(const char* text, const std::vector<file_dialog_filter>& filters)
{
    return "";
}

std::string get_username()
{
    return "unknown";
}

}; // namespace ws
