// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_fence.h"

namespace ws {

void vulkan_ri_fence::wait(size_t value)
{
}

void vulkan_ri_fence::wait(ri_command_queue& queue, size_t value)
{
}

size_t vulkan_ri_fence::current_value()
{
    return m_value;
}

void vulkan_ri_fence::signal(size_t value)
{
    m_value = value;
}

void vulkan_ri_fence::signal(ri_command_queue& queue, size_t value)
{
    m_value = value;
}

}; // namespace ws
