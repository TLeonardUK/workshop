// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_heap.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"

namespace ws {

namespace {

// Maps a bindless table type to the vulkan descriptor type used to back it. render_target
// and depth_stencil have no vulkan descriptor equivalent (dynamic rendering takes
// VkImageViews directly), so they're intentionally absent from this mapping.
bool descriptor_type_for_table(ri_descriptor_table table_type, VkDescriptorType& out_type)
{
    switch (table_type)
    {
    case ri_descriptor_table::texture_1d:
    case ri_descriptor_table::texture_2d:
    case ri_descriptor_table::texture_3d:
    case ri_descriptor_table::texture_cube:
        out_type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        return true;
    case ri_descriptor_table::sampler:
        out_type = VK_DESCRIPTOR_TYPE_SAMPLER;
        return true;
    case ri_descriptor_table::buffer:
    case ri_descriptor_table::rwbuffer:
        out_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        return true;
    case ri_descriptor_table::rwtexture_2d:
        out_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        return true;
    case ri_descriptor_table::tlas:
        out_type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        return true;
    default:
        return false;
    }
}

}; // namespace

vulkan_ri_descriptor_heap::vulkan_ri_descriptor_heap(vulkan_render_interface& renderer, ri_descriptor_table table_type, size_t size)
    : m_renderer(renderer)
    , m_table_type(table_type)
    , m_size(size)
{
}

vulkan_ri_descriptor_heap::~vulkan_ri_descriptor_heap()
{
    VkDevice device = m_renderer.get_device();

    if (m_pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }

    if (m_layout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device, m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }
}

result<void> vulkan_ri_descriptor_heap::create_resources()
{
    if (!descriptor_type_for_table(m_table_type, m_descriptor_type))
    {
        // No vulkan descriptor equivalent for this table type (render_target/depth_stencil).
        return true;
    }

    VkDescriptorPoolSize pool_size = {};
    pool_size.type = m_descriptor_type;
    pool_size.descriptorCount = static_cast<uint32_t>(m_size);

    VkDescriptorPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    pool_create_info.maxSets = 1;
    pool_create_info.poolSizeCount = 1;
    pool_create_info.pPoolSizes = &pool_size;

    VkResult vk_result = vkCreateDescriptorPool(m_renderer.get_device(), &pool_create_info, nullptr, &m_pool);
    if (!m_renderer.check_result(vk_result, "vkCreateDescriptorPool"))
    {
        return standard_errors::failed;
    }

    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = m_descriptor_type;
    binding.descriptorCount = static_cast<uint32_t>(m_size);
    binding.stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorBindingFlags binding_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_create_info = {};
    binding_flags_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    binding_flags_create_info.bindingCount = 1;
    binding_flags_create_info.pBindingFlags = &binding_flags;

    VkDescriptorSetLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.pNext = &binding_flags_create_info;
    layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layout_create_info.bindingCount = 1;
    layout_create_info.pBindings = &binding;

    vk_result = vkCreateDescriptorSetLayout(m_renderer.get_device(), &layout_create_info, nullptr, &m_layout);
    if (!m_renderer.check_result(vk_result, "vkCreateDescriptorSetLayout"))
    {
        return standard_errors::failed;
    }

    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = m_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &m_layout;

    vk_result = vkAllocateDescriptorSets(m_renderer.get_device(), &allocate_info, &m_set);
    if (!m_renderer.check_result(vk_result, "vkAllocateDescriptorSets"))
    {
        return standard_errors::failed;
    }

    return true;
}

VkDescriptorType vulkan_ri_descriptor_heap::get_descriptor_type()
{
    return m_descriptor_type;
}

VkDescriptorSet vulkan_ri_descriptor_heap::get_descriptor_set()
{
    return m_set;
}

VkDescriptorSetLayout vulkan_ri_descriptor_heap::get_descriptor_set_layout()
{
    return m_layout;
}

size_t vulkan_ri_descriptor_heap::get_size()
{
    return m_size;
}

}; // namespace ws
