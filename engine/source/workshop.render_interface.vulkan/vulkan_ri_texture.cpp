// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_texture.h"

#include <utility>

namespace ws {

vulkan_ri_texture::vulkan_ri_texture(const create_params& params, const char* debug_name)
    : m_params(params)
    , m_debug_name(debug_name ? debug_name : "")
{
}

size_t vulkan_ri_texture::get_width()
{
    return m_params.width;
}

size_t vulkan_ri_texture::get_pitch()
{
    return m_params.width;
}

size_t vulkan_ri_texture::get_height()
{
    return m_params.height;
}

size_t vulkan_ri_texture::get_depth()
{
    return m_params.depth;
}

size_t vulkan_ri_texture::get_mip_levels()
{
    return m_params.mip_levels;
}

size_t vulkan_ri_texture::get_dropped_mips()
{
    return m_params.drop_mips;
}

ri_texture_dimension vulkan_ri_texture::get_dimensions() const
{
    return m_params.dimensions;
}

ri_texture_format vulkan_ri_texture::get_format()
{
    return m_params.format;
}

size_t vulkan_ri_texture::get_multisample_count()
{
    return m_params.multisample_count;
}

color vulkan_ri_texture::get_optimal_clear_color()
{
    return m_params.optimal_clear_color;
}

float vulkan_ri_texture::get_optimal_clear_depth()
{
    return m_params.optimal_clear_depth;
}

uint8_t vulkan_ri_texture::get_optimal_clear_stencil()
{
    return m_params.optimal_clear_stencil;
}

bool vulkan_ri_texture::is_render_target()
{
    return m_params.is_render_target;
}

bool vulkan_ri_texture::is_depth_stencil()
{
    return false;
}

bool vulkan_ri_texture::is_partially_resident() const
{
    return m_params.is_partially_resident;
}

size_t vulkan_ri_texture::get_resident_mips()
{
    return m_params.resident_mips;
}

void vulkan_ri_texture::make_mip_resident(size_t mip_index, const std::span<uint8_t>& linear_data)
{
}

void vulkan_ri_texture::make_mip_resident(size_t mip_index, ri_staging_buffer& data_buffer)
{
}

void vulkan_ri_texture::make_mip_non_resident(size_t mip_index)
{
}

size_t vulkan_ri_texture::get_memory_usage_with_residency(size_t mip_count)
{
    return 0;
}

bool vulkan_ri_texture::is_mip_resident(size_t mip_index)
{
    return true;
}

void vulkan_ri_texture::get_mip_source_data_range(size_t mip_index, size_t& offset, size_t& size)
{
    offset = 0;
    size = 0;
}

void vulkan_ri_texture::begin_mip_residency_change()
{
}

void vulkan_ri_texture::end_mip_residency_change()
{
}

ri_resource_state vulkan_ri_texture::get_initial_state()
{
    return ri_resource_state::initial;
}

const char* vulkan_ri_texture::get_debug_name()
{
    return m_debug_name.c_str();
}

void vulkan_ri_texture::swap(ri_texture* other)
{
    vulkan_ri_texture* other_texture = static_cast<vulkan_ri_texture*>(other);
    std::swap(m_params, other_texture->m_params);
    std::swap(m_debug_name, other_texture->m_debug_name);
}

}; // namespace ws
