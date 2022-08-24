// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/debug/log_handler.h"

#include <array>
#include <algorithm>
#include <ctime>
#include <stdarg.h>

namespace ws {

std::array<const char*, static_cast<int>(log_level::count)> log_level_strings = {
    "fatal",
    "error",
    "warning",
    "success",
    "log",
    "verbose"
};

std::array<const char*, static_cast<int>(log_source::count)> log_source_strings = {
    "core",
    "engine",
    "game",
    "window",
    "render interface",
    "renderer",
    "asset"
};

void log_handler::set_max_log_level(log_level level)
{
    m_max_log_level = level;
}

log_handler::log_handler()
{
    std::scoped_lock lock(m_handlers_mutex);

    m_handlers.push_back(this);
}

log_handler::~log_handler()
{
    std::scoped_lock lock(m_handlers_mutex);

    if (auto iter = std::find(m_handlers.begin(), m_handlers.end(), this); iter != m_handlers.end())
    {
        m_handlers.erase(iter);
    }
}

void log_handler::static_write_formatted(log_level level, log_source source, const char* log)
{
    char buffer[k_stack_space];
    char* buffer_to_use = buffer;

    time_t current_time = time(0);
    struct tm current_time_tstruct;
    localtime_s(&current_time_tstruct, &current_time);

    char time_buffer[32];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %X", &current_time_tstruct);

    int ret = snprintf(buffer_to_use, 256, "%s \xB3 %-7s \xB3 %-18s \xB3 %s\n", time_buffer, log_level_strings[(int)level], log_source_strings[(int)source], log);
    if (ret >= 256)
    {
        buffer_to_use = new char[ret + 1];
        snprintf(buffer_to_use, ret + 1, "%s \xB3 %-7s \xB3 %-18s \xB3 %s\n", time_buffer, log_level_strings[(int)level], log_source_strings[(int)source], log);
    }

    {
        std::scoped_lock lock(m_handlers_mutex);

        for (log_handler* handler : m_handlers)
        {
            handler->write(level, buffer_to_use);
        }
    }

    if (buffer_to_use != buffer)
    {
        delete[] buffer_to_use;
    }
}

void log_handler::static_write(log_level level, log_source source, const char* format, ...)
{
    if (level > m_max_log_level)
    {
        return;
    }

    char buffer[k_stack_space];
    char* buffer_to_use = buffer;

    va_list list;
    va_start(list, format);

    int ret = vsnprintf(buffer_to_use, k_stack_space, format, list);
    int space_required = ret + 1;
    if (ret >= k_stack_space)
    {
        buffer_to_use = new char[space_required];
        vsnprintf(buffer_to_use, space_required, format, list);
    }

    static_write_formatted(level, source, buffer_to_use);

    if (buffer_to_use != buffer)
    {
        delete[] buffer_to_use;
    }

    va_end(list);
}

}; // namespace workshop
