// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/cvar/cvar.h"
#include "workshop.core/platform/platform.h"

namespace ws {

void register_core_cvars();

// ================================================================================================
//  Read-Only properties used for configuration files.
// ================================================================================================

inline cvar<std::string> cvar_platform(
    cvar_flag::read_only,
    platform_type_strings[(int)get_platform()],
    "platform",
    "Name of platform running on."
);

inline cvar<std::string> cvar_config(
    cvar_flag::read_only,
    config_type_strings[(int)get_config()],
    "config",
    "Configuration the build has been compiled in."
);

inline cvar<int> cvar_cpu_memory(
    cvar_flag::read_only,
    (int)(get_total_memory() / (1024llu * 1024llu)),
    "cpu_memory",
    "Number of megabytes of ram installed on the machine."
);

}; // namespace ws
