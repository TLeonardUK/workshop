// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/perf/profile.h"
#include "workshop.core/filesystem/file.h"

#include "workshop.core.win32/utils/windows_headers.h"
#include "thirdparty/pix/include/pix3.h"

#include <filesystem>
#include <shlobj.h>

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
