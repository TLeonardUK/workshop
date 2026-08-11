// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_table.h"
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_heap.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"

namespace ws {

vulkan_ri_descriptor_table::vulkan_ri_descriptor_table(vulkan_render_interface& renderer, ri_descriptor_table table_type)
    : m_renderer(renderer)
    , m_table_type(table_type)
{
    m_size = vulkan_render_interface::k_descriptor_table_sizes[(int)m_table_type];
    m_heap = std::make_unique<vulkan_ri_descriptor_heap>(m_renderer, m_table_type, m_size);
}

vulkan_ri_descriptor_table::~vulkan_ri_descriptor_table()
{
}

result<void> vulkan_ri_descriptor_table::create_resources()
{
    if (result<void> ret = m_heap->create_resources(); !ret)
    {
        return ret;
    }

    m_free_list.resize(m_size);
    for (size_t i = 0; i < m_size; i++)
    {
        m_free_list[i] = i;
    }

    return true;
}

vulkan_ri_descriptor_table::allocation vulkan_ri_descriptor_table::allocate()
{
    std::scoped_lock lock(m_mutex);

    allocation alloc;

    if (m_free_list.empty())
    {
        db_fatal(render_interface, "Descriptor table ran out of descriptors to allocate!");
    }

    alloc.is_valid = true;
    alloc.index = m_free_list.back();

    m_free_list.pop_back();

    return alloc;
}

void vulkan_ri_descriptor_table::free(allocation alloc)
{
    std::scoped_lock lock(m_mutex);

    m_free_list.push_back(alloc.index);
}

void vulkan_ri_descriptor_table::write_sampled_image(allocation alloc, VkImageView view)
{
    VkDescriptorImageInfo image_info = {};
    image_info.imageView = view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_heap->get_descriptor_set();
    write.dstBinding = 0;
    write.dstArrayElement = static_cast<uint32_t>(alloc.index);
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(m_renderer.get_device(), 1, &write, 0, nullptr);
}

void vulkan_ri_descriptor_table::write_storage_image(allocation alloc, VkImageView view)
{
    VkDescriptorImageInfo image_info = {};
    image_info.imageView = view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_heap->get_descriptor_set();
    write.dstBinding = 0;
    write.dstArrayElement = static_cast<uint32_t>(alloc.index);
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(m_renderer.get_device(), 1, &write, 0, nullptr);
}

void vulkan_ri_descriptor_table::write_storage_buffer(allocation alloc, VkBuffer buffer, size_t offset, size_t size)
{
    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = buffer;
    buffer_info.offset = offset;
    buffer_info.range = size;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_heap->get_descriptor_set();
    write.dstBinding = 0;
    write.dstArrayElement = static_cast<uint32_t>(alloc.index);
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(m_renderer.get_device(), 1, &write, 0, nullptr);
}

void vulkan_ri_descriptor_table::write_sampler(allocation alloc, VkSampler sampler)
{
    VkDescriptorImageInfo image_info = {};
    image_info.sampler = sampler;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_heap->get_descriptor_set();
    write.dstBinding = 0;
    write.dstArrayElement = static_cast<uint32_t>(alloc.index);
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(m_renderer.get_device(), 1, &write, 0, nullptr);
}

void vulkan_ri_descriptor_table::write_acceleration_structure(allocation alloc, VkAccelerationStructureKHR structure)
{
    VkWriteDescriptorSetAccelerationStructureKHR as_write = {};
    as_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    as_write.accelerationStructureCount = 1;
    as_write.pAccelerationStructures = &structure;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = &as_write;
    write.dstSet = m_heap->get_descriptor_set();
    write.dstBinding = 0;
    write.dstArrayElement = static_cast<uint32_t>(alloc.index);
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    vkUpdateDescriptorSets(m_renderer.get_device(), 1, &write, 0, nullptr);
}

VkDescriptorSet vulkan_ri_descriptor_table::get_descriptor_set()
{
    return m_heap->get_descriptor_set();
}

VkDescriptorSetLayout vulkan_ri_descriptor_table::get_descriptor_set_layout()
{
    return m_heap->get_descriptor_set_layout();
}

size_t vulkan_ri_descriptor_table::get_used_count()
{
    std::scoped_lock lock(m_mutex);
    return m_size - m_free_list.size();
}

size_t vulkan_ri_descriptor_table::get_total_count()
{
    return m_size;
}

}; // namespace ws
