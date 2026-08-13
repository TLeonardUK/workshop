// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"

namespace ws {

vulkan_ri_command_queue::vulkan_ri_command_queue(vulkan_render_interface& renderer, const char* debug_name, int queue_family)
{ 
}

vulkan_ri_command_queue::~vulkan_ri_command_queue()
{ 
}

result<void> vulkan_ri_command_queue::create_resources()
{ 
    return true;
}

ri_command_list& vulkan_ri_command_queue::alloc_command_list()
{
    m_command_lists.push_back(std::make_unique<vulkan_ri_command_list>());
    return *m_command_lists.back();
}

void vulkan_ri_command_queue::execute(ri_command_list& list)
{
}

void vulkan_ri_command_queue::execute(const std::vector<ri_command_list*>& list)
{
}

void vulkan_ri_command_queue::begin_event(const color& color, const char* name, ...)
{
}

void vulkan_ri_command_queue::end_event()
{
}

void vulkan_ri_command_queue::begin_frame()
{ 
}

void vulkan_ri_command_queue::end_frame()
{ 
}

}; // namespace ws
