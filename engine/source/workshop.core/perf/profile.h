// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/drawing/color.h"

#include <string>
#include <utility>

namespace ws {

namespace profile_colors {

// Color to use for the highest level engine markers (individual frames, etc)
inline static const color engine = color::green;

// Color to use for the highest level render markers.
inline static const color render = color::red;

// Color to use for the highest level simulation markers.
inline static const color simulation = color::blue;

// Color to use for high level systems (particle update, etc).
inline static const color system = color::orange;

// Color to use for leaf tasks.
inline static const color task = color::grey;

// Color to use for wait/stall events.
inline static const color wait = color::black;

}; // namespace profile_color.

// ================================================================================================
//  Emits a profile marker start using the platforms supported profiler api.
//  Do not call directly, use profile_maker instead to compile-out in release.
// ================================================================================================
void platform_perf_begin_marker(const color& color, const char* format, ...);

// ================================================================================================
//  Emits a profile marker end using the platforms supported profiler api.
//  Do not call directly, use profile_maker instead to compile-out in release.
// ================================================================================================
void platform_perf_end_marker();

// ================================================================================================
//  Emits a profile variable using the platforms supported profiler api.
//  Do not call directly, use profile_maker instead to compile-out in release.
// ================================================================================================
void platform_perf_variable(double value, const char* format, ...);

// ================================================================================================

template <typename... Args>
struct scoped_profile_marker
{
    scoped_profile_marker(const color& color, const char* format, Args&&... args)
    {
        platform_perf_begin_marker(color, format, std::forward<Args>(args)...);
    }
    ~scoped_profile_marker()
    {
        platform_perf_end_marker();
    }
};

#ifdef WS_RELEASE
#define profile_marker(color, name, ...)
#define profile_variable(value, name, ...)
#else
#define profile_marker(color, name, ...)        scoped_profile_marker pm_##__LINE__(color, name, __VA_ARGS__)
#define profile_variable(value, name, ...)      platform_perf_variable(value, name, __VA_ARGS__)
#endif

}; // namespace workshop
