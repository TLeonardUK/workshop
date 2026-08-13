// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_param_block.h"

namespace ws {

vulkan_ri_param_block::vulkan_ri_param_block(ri_param_block_archetype* archetype)
    : m_archetype(archetype)
{
}

bool vulkan_ri_param_block::set(string_hash field_name, const ri_texture& resource)
{
    return true;
}

bool vulkan_ri_param_block::set(string_hash field_name, const ri_texture_view& resource, bool writable)
{
    return true;
}

bool vulkan_ri_param_block::set(string_hash field_name, const ri_sampler& resource)
{
    return true;
}

bool vulkan_ri_param_block::set(string_hash field_name, const ri_buffer& resource, bool writable)
{
    return true;
}

bool vulkan_ri_param_block::set(string_hash field_name, const ri_raytracing_tlas& resource)
{
    return true;
}

bool vulkan_ri_param_block::clear_buffer(string_hash field_name)
{
    return true;
}

ri_param_block_archetype* vulkan_ri_param_block::get_archetype()
{
    return m_archetype;
}

void vulkan_ri_param_block::get_table(size_t& index, size_t& offset)
{
    index = 0;
    offset = 0;
}

bool vulkan_ri_param_block::set(string_hash field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type)
{
    return true;
}

void vulkan_ri_param_block::upload_state()
{ 
}

}; // namespace ws
