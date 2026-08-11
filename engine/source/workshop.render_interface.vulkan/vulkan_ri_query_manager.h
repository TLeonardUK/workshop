// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.render_interface/ri_query.h"
#include "workshop.core/utils/result.h"
#include "workshop.core/memory/memory_tracker.h"

#include <mutex>
#include <vector>

namespace ws {

class vulkan_render_interface;

// ================================================================================================
//  Handles management and resolving of query data.
// ================================================================================================
class vulkan_ri_query_manager
{
public:
    using query_id = size_t;

    static inline const size_t k_invalid_query_id = std::numeric_limits<size_t>::max();

public:
    vulkan_ri_query_manager(vulkan_render_interface& renderer, size_t max_queries);
    ~vulkan_ri_query_manager();

    result<void> create_resources();

    void begin_frame();

    query_id allocate_query();
    void free_query(query_id id);

    void begin(query_id id, VkCommandBuffer command_buffer);
    void end(query_id id, VkCommandBuffer command_buffer);

    bool are_results_ready(query_id id);
    double get_results(query_id id);

private:
    struct query_info
    {
        ri_query_type type = ri_query_type::time;
        size_t started_frame = std::numeric_limits<size_t>::max();
    };

private:
    vulkan_render_interface& m_renderer;
    size_t m_max_queries;
    size_t m_query_slots = 0;
    size_t m_pipeline_depth = 0;

    std::mutex m_mutex;

    std::vector<query_info> m_query_info;
    std::vector<query_id> m_free_queries;
    std::vector<uint64_t> m_read_back_times;

    VkQueryPool m_query_pool = VK_NULL_HANDLE;

    VkBuffer m_readback_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_readback_memory = VK_NULL_HANDLE;
    uint8_t* m_readback_ptr = nullptr;

    std::unique_ptr<memory_allocation> m_memory_allocation_info = nullptr;

    size_t m_resolve_frame_index = 0;

    double m_timestamp_period_ns = 1.0;

};

}; // namespace ws
