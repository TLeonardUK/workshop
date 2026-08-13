// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_small_buffer_allocator.h"

namespace ws {

vulkan_ri_small_buffer_allocator::vulkan_ri_small_buffer_allocator(vulkan_render_interface& renderer)
{
}

vulkan_ri_small_buffer_allocator::~vulkan_ri_small_buffer_allocator()
{ 
}

result<void> vulkan_ri_small_buffer_allocator::create_resources()
{
    return true;
}

bool vulkan_ri_small_buffer_allocator::alloc(size_t size, ri_buffer_usage usage, handle& out_handle)
{
    return false;
}

void vulkan_ri_small_buffer_allocator::free(handle in_handle)
{
}

size_t vulkan_ri_small_buffer_allocator::get_max_size()
{
    return 0;
}

}; // namespace ws
