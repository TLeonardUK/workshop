// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_swapchain.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_texture.h"
#include "workshop.window_interface/window.h"
#include "workshop.core/utils/result.h"
#include "workshop.core/memory/memory_tracker.h"

#include <array>
#include <string>

namespace ws {

class ri_fence;

// ================================================================================================
//  Implementation of a swapchain using Vulkan.
// ================================================================================================
class vulkan_ri_swapchain : public ri_swapchain
{
public:
    vulkan_ri_swapchain(vulkan_render_interface& renderer, window& for_window, const char* debug_name);
    virtual ~vulkan_ri_swapchain();

    result<void> create_resources();

    virtual ri_texture& next_backbuffer() override;
    virtual void present() override;
    virtual void drain() override;

private:
    void destroy_resources();
    result<void> resize_buffers();
    result<void> create_render_targets();

private:
    // Minus one to account for the frame currently being generated. This is the number of
    // images we'd prefer, but the surface's actual minImageCount may force more (see
    // create_resources) - k_max_buffer_count is the hard upper bound the fixed-size arrays
    // below are sized to, m_image_count is how many are actually in use at runtime.
    constexpr static size_t k_buffer_count = vulkan_render_interface::k_max_pipeline_depth - 1;
    constexpr static size_t k_max_buffer_count = 8;

    vulkan_render_interface& m_renderer;
    window& m_window;
    std::string m_debug_name;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkFormat m_view_format = VK_FORMAT_UNDEFINED;
    ri_texture_format m_ri_format = ri_texture_format::R8G8B8A8_SRGB;
    bool m_needs_mutable_format = false;

    size_t m_image_count = 0;
    std::array<std::unique_ptr<vulkan_ri_texture>, k_max_buffer_count> m_back_buffer_targets;
    std::array<size_t, k_max_buffer_count> m_back_buffer_last_used_frame = {};

    VkFence m_acquire_fence = VK_NULL_HANDLE;

    std::unique_ptr<ri_fence> m_fence;

    size_t m_current_buffer_index = 0;
    size_t m_frame_index = 1;

    size_t m_window_width = 0;
    size_t m_window_height = 0;
    window_mode m_window_mode = window_mode::windowed;

    std::unique_ptr<memory_allocation> m_memory_allocation_info = nullptr;

};

}; // namespace ws
