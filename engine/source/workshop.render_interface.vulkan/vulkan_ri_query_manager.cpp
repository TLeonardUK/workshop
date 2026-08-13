// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_query_manager.h"

namespace ws {

vulkan_ri_query_manager::vulkan_ri_query_manager(vulkan_render_interface& renderer, size_t max_queries)
{
}

vulkan_ri_query_manager::~vulkan_ri_query_manager()
{ 
}

result<void> vulkan_ri_query_manager::create_resources()
{
    return true;
}

void vulkan_ri_query_manager::begin_frame()
{
}

vulkan_ri_query_manager::query_id vulkan_ri_query_manager::allocate_query()
{
    return k_invalid_query_id;
}

void vulkan_ri_query_manager::free_query(query_id id)
{
}

void vulkan_ri_query_manager::begin(query_id id, VkCommandBuffer command_buffer)
{
}

void vulkan_ri_query_manager::end(query_id id, VkCommandBuffer command_buffer)
{
}

bool vulkan_ri_query_manager::are_results_ready(query_id id)
{
    return true;
}

double vulkan_ri_query_manager::get_results(query_id id)
{
    return 0.0;
}

}; // namespace ws
