// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/cvar/core_cvars.h"
#include "workshop.core/cvar/cvar_manager.h"
#include "workshop.core/debug/debug.h"
#include "workshop.core/containers/string.h"

namespace ws {

void register_core_cvars()
{
    cvar_platform.register_self();
    cvar_config.register_self();
    cvar_cpu_memory.register_self();
}

}; // namespace ws
