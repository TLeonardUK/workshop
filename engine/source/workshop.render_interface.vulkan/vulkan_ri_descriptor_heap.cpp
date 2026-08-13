// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_heap.h"

namespace ws {

vulkan_ri_descriptor_heap::vulkan_ri_descriptor_heap(vulkan_render_interface& renderer, ri_descriptor_table table_type, size_t size)
    : m_table_type(table_type)
    , m_size(size)
{
}

vulkan_ri_descriptor_heap::~vulkan_ri_descriptor_heap() = default;

result<void> vulkan_ri_descriptor_heap::create_resources()
{
    return true;
}

VkDescriptorType vulkan_ri_descriptor_heap::get_descriptor_type()
{
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

VkDescriptorSet vulkan_ri_descriptor_heap::get_descriptor_set()
{
    return VK_NULL_HANDLE;
}

VkDescriptorSetLayout vulkan_ri_descriptor_heap::get_descriptor_set_layout()
{
    return VK_NULL_HANDLE;
}

size_t vulkan_ri_descriptor_heap::get_size()
{
    return m_size;
}

}; // namespace ws
