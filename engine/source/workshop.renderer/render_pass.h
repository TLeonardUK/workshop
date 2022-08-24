// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

namespace ws {

class render_effect;

// ================================================================================================
//  Base class for all render passes.
// 
//  A render pass represents one or more draw calls that all run using the same pipeline state
//  and parameters. 
// ================================================================================================
class render_pass
{
public:

    // Debugging name for this pass, will show up in profiles.
    std::string name;

    // The effect that should be used during this pass. 
    render_effect* effect;


};

}; // namespace ws
