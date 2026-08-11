// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_fence.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.vulkan/vulkan_headers.h"

#include <string>

namespace ws {

class vulkan_render_interface;

// ================================================================================================
//  Implementation of a fence using Vulkan.
// ================================================================================================
class vulkan_ri_fence : public ri_fence
{
public:
    vulkan_ri_fence(vulkan_render_interface& renderer, const char* debug_name);
    virtual ~vulkan_ri_fence();

    // Creates the vulkan resources required by this fence.
    result<void> create_resources();

    virtual void wait(size_t value) override;
    virtual void wait(ri_command_queue& queue, size_t value) override;
    virtual size_t current_value() override;
    virtual void signal(size_t value) override;
    virtual void signal(ri_command_queue& queue, size_t value) override;

    VkSemaphore get_semaphore();

private:
    vulkan_render_interface& m_renderer;
    std::string m_debug_name;

    VkSemaphore m_semaphore = VK_NULL_HANDLE;

};

}; // namespace ws
