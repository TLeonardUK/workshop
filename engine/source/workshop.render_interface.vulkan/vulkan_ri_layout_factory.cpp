// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_layout_factory.h"
#include "workshop.render_interface.vulkan/vulkan_ri_buffer.h"

namespace ws {

vulkan_ri_layout_factory::vulkan_ri_layout_factory(ri_data_layout layout, ri_layout_usage usage)
    : m_layout(layout)
    , m_usage(usage)
{
}

void vulkan_ri_layout_factory::clear()
{
    m_instance_size = 0;
}

size_t vulkan_ri_layout_factory::get_instance_size()
{
    return m_instance_size;
}

std::unique_ptr<ri_buffer> vulkan_ri_layout_factory::create_vertex_buffer(const char* name)
{
    ri_buffer::create_params params;
    params.usage = ri_buffer_usage::vertex_buffer;
    params.element_count = 1;
    params.element_size = m_instance_size;
    return std::make_unique<vulkan_ri_buffer>(params, name);
}

std::unique_ptr<ri_buffer> vulkan_ri_layout_factory::create_index_buffer(const char* name, const std::vector<uint32_t>& indices)
{
    ri_buffer::create_params params;
    params.usage = ri_buffer_usage::index_buffer;
    params.element_count = indices.size();
    params.element_size = sizeof(uint32_t);
    return std::make_unique<vulkan_ri_buffer>(params, name);
}

void vulkan_ri_layout_factory::add(string_hash field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type)
{
    m_instance_size += value_size;
}

}; // namespace ws
