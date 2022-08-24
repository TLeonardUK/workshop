// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/perf/profile.h"

#include "workshop.core.win32/utils/windows_headers.h"
#include "thirdparty/pix/include/pix3.h"

namespace ws {

void platform_perf_begin_marker(const color& color, const char* format, ...)
{
    uint8_t r, g, b, a;
    color.get(r, g, b, a);

    char buffer[1024];
    
    va_list list;
    va_start(list, format);

    int ret = vsnprintf(buffer, sizeof(buffer), format, list);
    int space_required = ret + 1;
    if (ret >= sizeof(buffer))
    {
        return;
    }

    PIXBeginEvent(PIX_COLOR(r, g, b), "%s", buffer);

    va_end(list);
}

void platform_perf_end_marker()
{
    PIXEndEvent();
}

void platform_perf_variable(double value, const char* format, ...)
{
    char buffer[1024];

    va_list list;
    va_start(list, format);

    int ret = vsnprintf(buffer, sizeof(buffer), format, list);
    int space_required = ret + 1;
    if (ret >= sizeof(buffer))
    {
        return;
    }

    PIXReportCounter(widen_string(buffer).c_str(), static_cast<float>(value));

    va_end(list);
}

}; // namespace workshop
