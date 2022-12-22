// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"

namespace ws {

class ri_param_block;

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

public:

    virtual std::unique_ptr<ri_param_block> create_param_block() = 0;

    virtual const char* get_name() = 0;

};

}; // namespace ws
