// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/perf/profile.h"
#include "workshop.core/filesystem/file.h"

#include <filesystem>

namespace ws {

void platform_perf_init()
{
#ifndef WS_RELEASE
    // linux-todo
#endif
}

void platform_perf_begin_marker(const color& color, const char* format, ...)
{
    // linux-todo
}

void platform_perf_end_marker()
{
    // linux-todo
}

void platform_perf_variable(double value, const char* format, ...)
{
    // linux-todo
}

}; // namespace workshop
