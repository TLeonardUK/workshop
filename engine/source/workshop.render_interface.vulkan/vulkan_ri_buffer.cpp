// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.core/memory/memory_tracker.h"

namespace ws {

// vulkan-todo

vulkan_ri_buffer::vulkan_ri_buffer(vulkan_render_interface& renderer, const char* debug_name, const ri_buffer::create_params& params)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
{
}

vulkan_ri_buffer::~vulkan_ri_buffer()
{
}

result<void> vulkan_ri_buffer::create_resources()
{
    return false;
}

size_t vulkan_ri_buffer::get_element_count()
{
    return m_create_params.element_count;
}

size_t vulkan_ri_buffer::get_element_size()
{
    return m_create_params.element_size;
}

const char* vulkan_ri_buffer::get_debug_name()
{
    return m_debug_name.c_str();
}

ri_resource_state vulkan_ri_buffer::get_initial_state()
{
    return m_common_state;
}

void* vulkan_ri_buffer::map(size_t offset, size_t size)
{
    return nullptr;
}

void vulkan_ri_buffer::unmap(void* pointer)
{
}

}; // namespace ws
