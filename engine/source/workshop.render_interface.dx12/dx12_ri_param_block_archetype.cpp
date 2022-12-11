// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_param_block_archetype.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"

namespace ws {

dx12_ri_param_block_archetype::dx12_ri_param_block_archetype(dx12_render_interface& renderer, const ri_param_block_archetype::create_params& params, const char* debug_name)
    : m_renderer(renderer)
    , m_create_params(params)
    , m_debug_name(debug_name)
{
}

dx12_ri_param_block_archetype::~dx12_ri_param_block_archetype()
{
}

result<void> dx12_ri_param_block_archetype::create_resources()
{
    // Nothing to do for this?

    return true;
}

}; // namespace ws
