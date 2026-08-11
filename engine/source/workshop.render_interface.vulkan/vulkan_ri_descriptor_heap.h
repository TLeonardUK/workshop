// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.render_interface/ri_types.h"
#include "workshop.core/utils/result.h"

namespace ws {

class vulkan_render_interface;

// ================================================================================================
//  Implementation of a super simple descriptor heap for vulkan.
// ================================================================================================
class vulkan_ri_descriptor_heap
{
public:
    vulkan_ri_descriptor_heap(vulkan_render_interface& renderer, ri_descriptor_table table_type, size_t size);
    ~vulkan_ri_descriptor_heap();

    result<void> create_resources();

    VkDescriptorType get_descriptor_type();
    VkDescriptorSet get_descriptor_set();
    VkDescriptorSetLayout get_descriptor_set_layout();
    size_t get_size();

private:
    vulkan_render_interface& m_renderer;
    ri_descriptor_table m_table_type;
    size_t m_size;

    VkDescriptorType m_descriptor_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

    VkDescriptorPool m_pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    VkDescriptorSet m_set = VK_NULL_HANDLE;

};

}; // namespace ws
