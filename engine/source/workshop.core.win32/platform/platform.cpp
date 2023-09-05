// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/platform/platform.h"

#include "thirdparty/nativefiledialog/src/include/nfd.h"

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

std::string open_file_dialog(const char* text, const std::vector<file_dialog_filter>& filters)
{
    std::vector<nfdu8filteritem_t> nfd_filters;
    std::vector<std::string> nfd_specs;

    nfd_filters.reserve(filters.size());
    nfd_specs.reserve(filters.size());

    for (const file_dialog_filter& filter : filters)
    {
        std::string& spec = nfd_specs.emplace_back();
        spec = string_join(filter.extensions, ",");

        nfdu8filteritem_t& item = nfd_filters.emplace_back();
        item.name = filter.name.c_str();
        item.spec = spec.data();
    }

    std::string result = "";

    nfdchar_t* path;
    nfdresult_t dialog_result = NFD_OpenDialog(&path, nfd_filters.data(), static_cast<nfdfiltersize_t>(nfd_filters.size()), nullptr);
    if (dialog_result == NFD_OKAY)
    {
        result = path;
        free(path);
    }

    return result;
}

std::string save_file_dialog(const char* text, const std::vector<file_dialog_filter>& filters)
{
    std::vector<nfdu8filteritem_t> nfd_filters;
    std::vector<std::string> nfd_specs;

    nfd_filters.reserve(filters.size());
    nfd_specs.reserve(filters.size());

    for (const file_dialog_filter& filter : filters)
    {
        std::string& spec = nfd_specs.emplace_back();
        spec = string_join(filter.extensions, ",");

        nfdu8filteritem_t& item = nfd_filters.emplace_back();
        item.name = filter.name.c_str();
        item.spec = spec.data();
    }

    std::string result = "";

    nfdchar_t* path;
    nfdresult_t dialog_result = NFD_SaveDialog(&path, nfd_filters.data(), static_cast<nfdfiltersize_t>(nfd_filters.size()), nullptr, nullptr);
    if (dialog_result == NFD_OKAY)
    {
        result = path;
        free(path);
    }

    return result;
}

}; // namespace workshop
