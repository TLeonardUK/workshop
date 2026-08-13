// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_table.h"

namespace ws {

vulkan_ri_descriptor_table::vulkan_ri_descriptor_table(vulkan_render_interface& renderer, ri_descriptor_table table_type)
    : m_table_type(table_type)
{
}

vulkan_ri_descriptor_table::~vulkan_ri_descriptor_table() = default;

result<void> vulkan_ri_descriptor_table::create_resources()
{
    return true;
}

vulkan_ri_descriptor_table::allocation vulkan_ri_descriptor_table::allocate()
{
    allocation alloc;
    alloc.is_valid = true;
    alloc.index = 0;
    return alloc;
}

void vulkan_ri_descriptor_table::free(allocation alloc)
{
}

void vulkan_ri_descriptor_table::write_sampled_image(allocation alloc, VkImageView view)
{
}

void vulkan_ri_descriptor_table::write_storage_image(allocation alloc, VkImageView view)
{
}

void vulkan_ri_descriptor_table::write_storage_buffer(allocation alloc, VkBuffer buffer, size_t offset, size_t size)
{
}

void vulkan_ri_descriptor_table::write_sampler(allocation alloc, VkSampler sampler)
{
}

void vulkan_ri_descriptor_table::write_acceleration_structure(allocation alloc, VkAccelerationStructureKHR structure)
{
}

}; // namespace ws
