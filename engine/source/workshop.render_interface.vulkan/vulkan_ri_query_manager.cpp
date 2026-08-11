// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_query_manager.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_list.h"

#include <cstring>

namespace ws {

vulkan_ri_query_manager::vulkan_ri_query_manager(vulkan_render_interface& renderer, size_t max_queries)
    : m_renderer(renderer)
    , m_max_queries(max_queries)
{
}

vulkan_ri_query_manager::~vulkan_ri_query_manager()
{
    if (m_readback_memory != VK_NULL_HANDLE)
    {
        vkUnmapMemory(m_renderer.get_device(), m_readback_memory);
    }
    if (m_readback_buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_renderer.get_device(), m_readback_buffer, nullptr);
    }
    if (m_readback_memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_renderer.get_device(), m_readback_memory, nullptr);
    }
    if (m_query_pool != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(m_renderer.get_device(), m_query_pool, nullptr);
    }
}

result<void> vulkan_ri_query_manager::create_resources()
{
    memory_scope mem_scope(memory_type::rendering__vram__queries);

    m_pipeline_depth = m_renderer.get_pipeline_depth();
    m_query_slots = m_max_queries * 2;
    m_read_back_times.resize(m_query_slots);
    m_query_info.resize(m_max_queries);
    for (size_t i = 0; i < m_max_queries; i++)
    {
        m_query_info[i].started_frame = std::numeric_limits<size_t>::max();
        m_free_queries.push_back((m_max_queries - 1) - i);
    }

    VkQueryPoolCreateInfo query_pool_create_info = {};
    query_pool_create_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_create_info.queryCount = static_cast<uint32_t>(m_query_slots);

    VkResult vk_result = vkCreateQueryPool(m_renderer.get_device(), &query_pool_create_info, nullptr, &m_query_pool);
    if (!m_renderer.check_result(vk_result, "vkCreateQueryPool"))
    {
        return standard_errors::failed;
    }

    size_t readback_size = sizeof(uint64_t) * m_query_slots * m_pipeline_depth;

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = readback_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk_result = vkCreateBuffer(m_renderer.get_device(), &buffer_create_info, nullptr, &m_readback_buffer);
    if (!m_renderer.check_result(vk_result, "vkCreateBuffer"))
    {
        return standard_errors::failed;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(m_renderer.get_device(), m_readback_buffer, &memory_requirements);

    result<uint32_t> memory_type_index = m_renderer.find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!memory_type_index)
    {
        return standard_errors::failed;
    }

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = memory_type_index.get_result();

    vk_result = vkAllocateMemory(m_renderer.get_device(), &allocate_info, nullptr, &m_readback_memory);
    if (!m_renderer.check_result(vk_result, "vkAllocateMemory"))
    {
        return standard_errors::failed;
    }

    vk_result = vkBindBufferMemory(m_renderer.get_device(), m_readback_buffer, m_readback_memory, 0);
    if (!m_renderer.check_result(vk_result, "vkBindBufferMemory"))
    {
        return standard_errors::failed;
    }

    vk_result = vkMapMemory(m_renderer.get_device(), m_readback_memory, 0, readback_size, 0, reinterpret_cast<void**>(&m_readback_ptr));
    if (!m_renderer.check_result(vk_result, "vkMapMemory"))
    {
        return standard_errors::failed;
    }

    m_memory_allocation_info = mem_scope.record_alloc(memory_requirements.size);

    m_timestamp_period_ns = static_cast<double>(m_renderer.get_physical_device_properties().limits.timestampPeriod);

    // Every slot must be reset before its first write.
    vulkan_ri_command_queue& queue = static_cast<vulkan_ri_command_queue&>(m_renderer.get_graphics_queue());
    vulkan_ri_command_list& list = static_cast<vulkan_ri_command_list&>(queue.alloc_command_list());
    list.open();
    vkCmdResetQueryPool(list.get_command_buffer(), m_query_pool, 0, static_cast<uint32_t>(m_query_slots));
    list.close();
    queue.execute(list);

    return true;
}

vulkan_ri_query_manager::query_id vulkan_ri_query_manager::allocate_query()
{
    std::scoped_lock lock(m_mutex);

    if (m_free_queries.empty())
    {
        db_error(render_interface, "Ran out of gpu queries. Failed to allocate new timer, results may be unexpected.");
        return k_invalid_query_id;
    }

    query_id id = m_free_queries.back();
    m_free_queries.pop_back();

    m_query_info[id].started_frame = std::numeric_limits<size_t>::max();

    return id;
}

void vulkan_ri_query_manager::free_query(query_id id)
{
    std::scoped_lock lock(m_mutex);

    if (id == k_invalid_query_id)
    {
        return;
    }

    m_free_queries.push_back(id);
    m_query_info[id].started_frame = std::numeric_limits<size_t>::max();
}

bool vulkan_ri_query_manager::are_results_ready(query_id id)
{
    std::scoped_lock lock(m_mutex);

    if (id == k_invalid_query_id)
    {
        return true;
    }

    return m_renderer.get_frame_index() > m_query_info[id].started_frame + m_pipeline_depth;
}

double vulkan_ri_query_manager::get_results(query_id id)
{
    std::scoped_lock lock(m_mutex);

    if (id == k_invalid_query_id)
    {
        return 0.0;
    }

    query_info& info = m_query_info[id];

    switch (info.type)
    {
    case ri_query_type::time:
        {
            uint64_t start = m_read_back_times[id * 2];
            uint64_t end = m_read_back_times[id * 2 + 1];

            if (end <= start)
            {
                return 0.0;
            }

            return double(end - start) * m_timestamp_period_ns * 1e-9;
        }
    }

    return 0.0;
}

void vulkan_ri_query_manager::begin(query_id id, VkCommandBuffer command_buffer)
{
    std::scoped_lock lock(m_mutex);

    if (id == k_invalid_query_id)
    {
        return;
    }

    vkCmdWriteTimestamp2(command_buffer, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, m_query_pool, static_cast<uint32_t>(id) * 2);
}

void vulkan_ri_query_manager::end(query_id id, VkCommandBuffer command_buffer)
{
    std::scoped_lock lock(m_mutex);

    if (id == k_invalid_query_id)
    {
        return;
    }

    vkCmdWriteTimestamp2(command_buffer, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, m_query_pool, static_cast<uint32_t>(id) * 2 + 1);

    if (m_query_info[id].started_frame == std::numeric_limits<size_t>::max())
    {
        m_query_info[id].started_frame = m_renderer.get_frame_index();
    }
}

void vulkan_ri_query_manager::begin_frame()
{
    std::scoped_lock lock(m_mutex);

    if (m_query_pool == VK_NULL_HANDLE)
    {
        return;
    }

    size_t resolve_base_offset = m_resolve_frame_index * m_query_slots * sizeof(uint64_t);

    vulkan_ri_command_queue& queue = static_cast<vulkan_ri_command_queue&>(m_renderer.get_graphics_queue());

    vulkan_ri_command_list& list = static_cast<vulkan_ri_command_list&>(queue.alloc_command_list());
    list.open();
    vkCmdCopyQueryPoolResults(
        list.get_command_buffer(),
        m_query_pool,
        0,
        static_cast<uint32_t>(m_query_slots),
        m_readback_buffer,
        resolve_base_offset,
        sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT
    );
    // Reset immediately after resolving, so this frame's begin()/end() calls can safely
    // rewrite the same slots - see class comment.
    vkCmdResetQueryPool(list.get_command_buffer(), m_query_pool, 0, static_cast<uint32_t>(m_query_slots));
    list.close();
    queue.execute(list);

    size_t read_back_index = (m_resolve_frame_index + 1) % m_pipeline_depth;
    size_t read_back_offset = read_back_index * m_query_slots * sizeof(uint64_t);

    memcpy(m_read_back_times.data(), m_readback_ptr + read_back_offset, sizeof(uint64_t) * m_query_slots);

    m_resolve_frame_index = read_back_index;
}

}; // namespace ws
