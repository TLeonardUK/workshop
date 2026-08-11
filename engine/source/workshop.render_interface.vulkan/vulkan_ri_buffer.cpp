// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_table.h"

#include <cstring>

namespace ws {

vulkan_ri_buffer::vulkan_ri_buffer(vulkan_render_interface& renderer, const char* debug_name, const ri_buffer::create_params& params)
    : m_renderer(renderer)
    , m_debug_name(debug_name ? debug_name : "")
    , m_create_params(params)
{
}

vulkan_ri_buffer::~vulkan_ri_buffer()
{
    m_renderer.defer_delete([renderer = &m_renderer, handle = m_handle, memory = m_memory, mapped_ptr = m_mapped_ptr, srv = m_srv, uav = m_uav, srv_table = m_srv_table, uav_table = m_uav_table, is_small = m_is_small_buffer, small_handle = m_small_buffer_allocation]() mutable
    {
        if (srv.is_valid)
        {
            renderer->get_descriptor_table(srv_table).free(srv);
        }
        if (uav.is_valid)
        {
            renderer->get_descriptor_table(uav_table).free(uav);
        }
        if (is_small)
        {
            renderer->get_small_buffer_allocator().free(small_handle);
        }
        if (handle != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(renderer->get_device(), handle, nullptr);
        }
        if (memory != VK_NULL_HANDLE)
        {
            if (mapped_ptr != nullptr)
            {
                vkUnmapMemory(renderer->get_device(), memory);
            }
            vkFreeMemory(renderer->get_device(), memory, nullptr);
        }
    });
}

bool vulkan_ri_buffer::is_small_buffer()
{
    return m_is_small_buffer;
}

size_t vulkan_ri_buffer::get_buffer_offset()
{
    if (m_is_small_buffer)
    {
        return m_small_buffer_allocation.offset;
    }
    return 0;
}

result<void> vulkan_ri_buffer::create_exclusive_buffer()
{
    size_t total_size = m_create_params.element_count * m_create_params.element_size;

    VkBufferUsageFlags usage_flags =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (m_create_params.usage == ri_buffer_usage::index_buffer)
    {
        usage_flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    if (m_renderer.check_feature(ri_feature::raytracing))
    {
        if (m_create_params.usage == ri_buffer_usage::raytracing_as)
        {
            usage_flags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
        }
        else if (m_create_params.usage == ri_buffer_usage::raytracing_shader_binding_table)
        {
            usage_flags |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
        }
        else
        {
            // Any other buffer (vertex/index/generic/instance-data/scratch) may end up used
            // as BLAS/TLAS build input (eg. a model's vertex/index buffers passed to
            // vulkan_ri_raytracing_blas::update) - there's no way to know at creation time
            // whether a given buffer will be used this way, so this is applied unconditionally
            // whenever raytracing is supported, same as the always-on flags above.
            usage_flags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        }
    }

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = total_size;
    buffer_create_info.usage = usage_flags;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult vk_result = vkCreateBuffer(m_renderer.get_device(), &buffer_create_info, nullptr, &m_handle);
    if (!m_renderer.check_result(vk_result, "vkCreateBuffer"))
    {
        return standard_errors::failed;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(m_renderer.get_device(), m_handle, &memory_requirements);

    // Raytracing acceleration structures/scratch/SBT/instance-data are read by dedicated RT
    // traversal hardware, which - unlike normal shader memory access - is not well tested (and
    // may not be reliable, up to and including causing VK_ERROR_DEVICE_LOST) against memory
    // that isn't device-local. Prefer a device-local+host-visible type for these (typically a
    // small BAR-mapped heap) so they land in VRAM like a typical Vulkan RT application, falling
    // back to the plain host-visible type used for everything else in this engine if no such
    // type exists or the (usually small) device-local-visible heap can't fit this allocation.
    bool prefer_device_local =
        m_create_params.usage == ri_buffer_usage::raytracing_as ||
        m_create_params.usage == ri_buffer_usage::raytracing_as_scratch ||
        m_create_params.usage == ri_buffer_usage::raytracing_shader_binding_table ||
        m_create_params.usage == ri_buffer_usage::raytracing_as_instance_data;

    result<uint32_t> memory_type;
    if (prefer_device_local)
    {
        memory_type = m_renderer.find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
    if (!prefer_device_local || !memory_type)
    {
        memory_type = m_renderer.find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
    if (!memory_type)
    {
        return standard_errors::failed;
    }

    VkMemoryAllocateFlagsInfo allocate_flags_info = {};
    allocate_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    allocate_flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.pNext = &allocate_flags_info;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = memory_type.get_result();

    vk_result = vkAllocateMemory(m_renderer.get_device(), &allocate_info, nullptr, &m_memory);
    if (vk_result != VK_SUCCESS && prefer_device_local)
    {
        // Preferred device-local-visible heap is likely too small for this allocation - retry
        // with the plain host-visible type.
        result<uint32_t> fallback_memory_type = m_renderer.find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (fallback_memory_type && fallback_memory_type.get_result() != memory_type.get_result())
        {
            allocate_info.memoryTypeIndex = fallback_memory_type.get_result();
            vk_result = vkAllocateMemory(m_renderer.get_device(), &allocate_info, nullptr, &m_memory);
        }
    }
    if (!m_renderer.check_result(vk_result, "vkAllocateMemory"))
    {
        return standard_errors::failed;
    }

    vk_result = vkBindBufferMemory(m_renderer.get_device(), m_handle, m_memory, 0);
    if (!m_renderer.check_result(vk_result, "vkBindBufferMemory"))
    {
        return standard_errors::failed;
    }

    void* mapped = nullptr;
    vk_result = vkMapMemory(m_renderer.get_device(), m_memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    if (!m_renderer.check_result(vk_result, "vkMapMemory"))
    {
        return standard_errors::failed;
    }
    m_mapped_ptr = reinterpret_cast<uint8_t*>(mapped);

    return true;
}

result<void> vulkan_ri_buffer::create_resources()
{
    vulkan_ri_small_buffer_allocator& small_allocator = m_renderer.get_small_buffer_allocator();
    size_t total_size = m_create_params.element_size * m_create_params.element_count;

    // Param blocks expect to be linearly indexable inside their buffer so we don't
    // support small buffer optimization for them.
    if (m_create_params.usage == ri_buffer_usage::param_block)
    {
        m_is_small_buffer = false;
    }
    else
    {
        m_is_small_buffer = (total_size < small_allocator.get_max_size());
    }

    m_srv_table = ri_descriptor_table::buffer;
    m_uav_table = ri_descriptor_table::rwbuffer;

    switch (m_create_params.usage)
    {
    case ri_buffer_usage::index_buffer:
        m_common_state = ri_resource_state::index_buffer;
        break;
    case ri_buffer_usage::raytracing_as:
        m_common_state = ri_resource_state::raytracing_acceleration_structure;
        m_srv_table = ri_descriptor_table::tlas;
        break;
    case ri_buffer_usage::raytracing_shader_binding_table:
    case ri_buffer_usage::raytracing_as_instance_data:
        m_common_state = ri_resource_state::non_pixel_shader_resource;
        break;
    case ri_buffer_usage::raytracing_as_scratch:
        m_common_state = ri_resource_state::unordered_access;
        break;
    case ri_buffer_usage::vertex_buffer:
        m_common_state = ri_resource_state::pixel_shader_resource;
        break;
    case ri_buffer_usage::generic:
        m_common_state = ri_resource_state::pixel_shader_resource;
        break;
    case ri_buffer_usage::param_block:
        m_common_state = ri_resource_state::generic_read;
        break;
    case ri_buffer_usage::readback:
        m_common_state = ri_resource_state::copy_dest;
        break;
    default:
        m_common_state = ri_resource_state::pixel_shader_resource;
        break;
    }

    if (m_is_small_buffer)
    {
        if (!small_allocator.alloc(total_size, m_create_params.usage, m_small_buffer_allocation))
        {
            return standard_errors::failed;
        }
    }
    else
    {
        if (result<void> ret = create_exclusive_buffer(); !ret)
        {
            return ret;
        }
    }

    size_t sub_allocation_offset = get_buffer_offset();

    // Upload the linear data if any has been provided - since every buffer is persistently
    // host-mapped, this is a direct memcpy with no upload-manager round trip needed.
    if (!m_create_params.linear_data.empty())
    {
        uint8_t* destination = static_cast<uint8_t*>(map(0, m_create_params.linear_data.size()));
        memcpy(destination, m_create_params.linear_data.data(), m_create_params.linear_data.size());
        unmap(destination);
    }

    // Register a bindless SRV (buffer table) so this buffer can be read via
    // table_byte_buffers[idx].Load<T>(offset) from any shader.
    m_srv = m_renderer.get_descriptor_table(m_srv_table).allocate();
    if (m_create_params.usage != ri_buffer_usage::raytracing_as)
    {
        m_renderer.get_descriptor_table(m_srv_table).write_storage_buffer(m_srv, get_buffer(), sub_allocation_offset, total_size);
    }

    // Register a bindless UAV (rwbuffer table) for usages that may need unordered access.
    if (m_create_params.usage == ri_buffer_usage::generic ||
        m_create_params.usage == ri_buffer_usage::param_block ||
        m_create_params.usage == ri_buffer_usage::raytracing_as ||
        m_create_params.usage == ri_buffer_usage::raytracing_as_scratch ||
        m_create_params.usage == ri_buffer_usage::raytracing_as_instance_data ||
        m_create_params.usage == ri_buffer_usage::raytracing_shader_binding_table)
    {
        m_uav = m_renderer.get_descriptor_table(m_uav_table).allocate();
        m_renderer.get_descriptor_table(m_uav_table).write_storage_buffer(m_uav, get_buffer(), sub_allocation_offset, total_size);
    }

    return true;
}

VkDeviceAddress vulkan_ri_buffer::get_gpu_address()
{
    VkBufferDeviceAddressInfo address_info = {};
    address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    address_info.buffer = get_buffer();

    VkDeviceAddress address = vkGetBufferDeviceAddress(m_renderer.get_device(), &address_info);
    return address + get_buffer_offset();
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

VkBuffer vulkan_ri_buffer::get_buffer()
{
    if (m_is_small_buffer)
    {
        return static_cast<vulkan_ri_buffer*>(m_small_buffer_allocation.buffer)->get_buffer();
    }
    return m_handle;
}

ri_resource_state vulkan_ri_buffer::get_initial_state()
{
    return m_common_state;
}

vulkan_ri_descriptor_table::allocation vulkan_ri_buffer::get_srv() const
{
    db_assert(m_srv.is_valid);
    return m_srv;
}

vulkan_ri_descriptor_table::allocation vulkan_ri_buffer::get_uav() const
{
    db_assert(m_uav.is_valid);
    return m_uav;
}

vulkan_ri_small_buffer_allocator::handle vulkan_ri_buffer::get_small_buffer_allocation() const
{
    return m_small_buffer_allocation;
}

void* vulkan_ri_buffer::map(size_t offset, size_t size)
{
    if (m_is_small_buffer)
    {
        return static_cast<vulkan_ri_buffer*>(m_small_buffer_allocation.buffer)->map(m_small_buffer_allocation.offset + offset, size);
    }

    db_assert(m_mapped_ptr != nullptr);
    return m_mapped_ptr + offset;
}

void vulkan_ri_buffer::unmap(void* pointer)
{
    // Memory is persistently mapped and coherent - nothing to do.
}

}; // namespace ws
