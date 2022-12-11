// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"

namespace ws {

// ================================================================================================
//  Describes the layout and scope of a block of parameters that can be bound to a 
//  draw call.
// ================================================================================================
class ri_param_block_archetype
{
public:

    struct create_params
    {
        ri_data_layout layout;
        ri_data_scope scope;
    };

};

}; // namespace ws
