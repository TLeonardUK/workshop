// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/platform/platform.h"

#include "thirdparty/nativefiledialog/src/include/nfd.h"

#include <SDL.h>

#include <fstream>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <pwd.h>

namespace ws {

namespace {

size_t get_proc_status_field_kb(const char* field_name)
{
    std::ifstream file("/proc/self/status");
    std::string line;

    size_t field_name_len = strlen(field_name);

    while (std::getline(file, line))
    {
        if (line.compare(0, field_name_len, field_name) == 0)
        {
            size_t value_kb = 0;
            sscanf(line.c_str() + field_name_len, "%zu", &value_kb);
            return value_kb;
        }
    }

    return 0;
}

bool ensure_nfd_initialized()
{
    static bool initialized = (NFD_Init() == NFD_OKAY);
    return initialized;
}

}; // namespace

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
    return get_proc_status_field_kb("VmSize:") * 1024;
}

size_t get_total_memory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages < 0 || page_size < 0)
    {
        return 0;
    }
    return static_cast<size_t>(pages) * static_cast<size_t>(page_size);
}

size_t get_pagefile_usage()
{
    return get_proc_status_field_kb("VmSwap:") * 1024;
}

void message_dialog(const char* text, message_dialog_type type)
{
    std::string caption = "";
    Uint32 native_type = 0;

    switch (type)
    {
        case message_dialog_type::error:
        {
            caption = "Error";
            native_type = SDL_MESSAGEBOX_ERROR;
            break;
        }
        case message_dialog_type::warning:
        {
            caption = "Warning";
            native_type = SDL_MESSAGEBOX_WARNING;
            break;
        }
        case message_dialog_type::message:
        {
            caption = "Message";
            native_type = SDL_MESSAGEBOX_INFORMATION;
            break;
        }
    }

    SDL_MessageBoxButtonData button = {};
    button.flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    button.buttonid = 0;
    button.text = "OK";

    SDL_MessageBoxData data = {};
    data.flags = native_type;
    data.window = nullptr;
    data.title = caption.c_str();
    data.message = text;
    data.numbuttons = 1;
    data.buttons = &button;
    data.colorScheme = nullptr;

    int button_id = 0;
    if (SDL_ShowMessageBox(&data, &button_id) != 0)
    {
        db_error(core, "SDL_ShowMessageBox failed: %s", SDL_GetError());
    }
}

std::string open_file_dialog(const char* text, const std::vector<file_dialog_filter>& filters)
{
    if (!ensure_nfd_initialized())
    {
        return "";
    }

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
    if (!ensure_nfd_initialized())
    {
        return "";
    }

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

std::string get_username()
{
    passwd* entry = getpwuid(geteuid());
    if (entry != nullptr && entry->pw_name != nullptr)
    {
        return entry->pw_name;
    }
    return "unknown";
}

}; // namespace workshop
