// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.core/utils/result.h"

#include <limits>

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

};

}; // namespace ws
