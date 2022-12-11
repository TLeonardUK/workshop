// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_param_block_archetype.h"
#include "workshop.core/utils/result.h"

namespace ws {

class dx12_render_interface;

// ================================================================================================
//  Implementation of a param block archetype using DirectX 12.
// ================================================================================================
class dx12_ri_param_block_archetype : public ri_param_block_archetype
{
public:
    dx12_ri_param_block_archetype(dx12_render_interface& renderer, const ri_param_block_archetype::create_params& params, const char* debug_name);
    virtual ~dx12_ri_param_block_archetype();

    // Creates the dx12 resources required by this resource.
    result<void> create_resources();

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;

    ri_param_block_archetype::create_params m_create_params;

};

}; // namespace ws
