// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_param_block_archetype.h"
#include "workshop.render_interface.vulkan/vulkan_ri_param_block.h"

namespace ws {

vulkan_ri_param_block_archetype::vulkan_ri_param_block_archetype(const create_params& params, const char* debug_name)
    : m_params(params)
    , m_debug_name(debug_name ? debug_name : "")
{
}

std::unique_ptr<ri_param_block> vulkan_ri_param_block_archetype::create_param_block()
{
    return std::make_unique<vulkan_ri_param_block>(this);
}

const ri_param_block_archetype::create_params& vulkan_ri_param_block_archetype::get_create_params()
{
    return m_params;
}

const char* vulkan_ri_param_block_archetype::get_name()
{
    return m_debug_name.c_str();
}

size_t vulkan_ri_param_block_archetype::get_size()
{
    return 0;
}

}; // namespace ws
