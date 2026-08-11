// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_swapchain.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_fence.h"
#include "workshop.render_interface.vulkan/vulkan_types.h"
#include "workshop.core/perf/profile.h"
#include "workshop.core/containers/string.h"

namespace ws {

namespace {

// Picks the swapchain's storage format plus the format its image views should actually be
// created with, and the ri_texture_format that correctly represents the view format - so the
// rest of the engine (in particular the pipeline color attachment formats a resolve/present
// shader is built against, which assume R8G8B8A8_SRGB - see common.yaml's sdr_swapchain output
// target) agrees with what the swapchain images actually look like to shaders.
//
// If the surface doesn't directly support R8G8B8A8_SRGB (common on some Linux/Vulkan drivers),
// this falls back to creating the swapchain in R8G8B8A8_UNORM storage while still exposing
// R8G8B8A8_SRGB *views* via VK_KHR_swapchain_mutable_format, mirroring DX12's swapchain design
// (typeless storage, typed views) rather than quietly changing what format the rest of the
// engine thinks the swapchain is.
struct chosen_swapchain_format
{
    VkSurfaceFormatKHR surface_format;
    VkFormat view_format;
    ri_texture_format ri_format;
    bool needs_mutable_format;
};

chosen_swapchain_format choose_surface_format(vulkan_render_interface& renderer, VkSurfaceKHR surface)
{
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer.get_physical_device(), surface, &format_count, nullptr);

    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer.get_physical_device(), surface, &format_count, formats.data());

    for (const VkSurfaceFormatKHR& format : formats)
    {
        if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return { format, VK_FORMAT_R8G8B8A8_SRGB, ri_texture_format::R8G8B8A8_SRGB, false };
        }
    }

    if (renderer.is_swapchain_mutable_format_supported())
    {
        // VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM and VK_FORMAT_R8G8B8A8_SRGB are
        // all in the same Vulkan "32-bit" format compatibility class (same texel size/layout),
        // so any 4x8-bit UNORM storage format can be validly reinterpreted as an
        // R8G8B8A8_SRGB view via mutable format - including B8G8R8A8_UNORM, which is what many
        // Linux surfaces actually report instead of R8G8B8A8_UNORM. Note this only swaps which
        // format the *numeric* interpretation uses, not the byte order - if the surface's real
        // storage is B-G-R-A, viewing it as R8G8B8A8_SRGB will still read those same bytes as
        // if they were R-G-B-A, so red and blue end up swapped unless something downstream
        // corrects for it. That's a real but narrow visual bug, and preferable to the
        // alternative of every draw against the backbuffer failing pipeline format validation.
        for (VkFormat candidate : { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM })
        {
            for (const VkSurfaceFormatKHR& format : formats)
            {
                if (format.format == candidate && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    db_warning(render_interface, "Surface does not directly support VK_FORMAT_R8G8B8A8_SRGB, creating swapchain with VK_FORMAT_R8G8B8A8_SRGB views via VK_KHR_swapchain_mutable_format - if the surface's real storage format is not R-G-B-A order, colors will be channel-swapped.");
                    return { format, VK_FORMAT_R8G8B8A8_SRGB, ri_texture_format::R8G8B8A8_SRGB, true };
                }
            }
        }
    }

    for (const VkSurfaceFormatKHR& format : formats)
    {
        if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            db_warning(render_interface, "Surface does not directly support VK_FORMAT_R8G8B8A8_SRGB, falling back to VK_FORMAT_R8G8B8A8_UNORM.");
            return { format, VK_FORMAT_R8G8B8A8_UNORM, ri_texture_format::R8G8B8A8, false };
        }
    }

    db_warning(render_interface, "Surface does not directly support VK_FORMAT_R8G8B8A8_SRGB or VK_FORMAT_R8G8B8A8_UNORM, falling back to first available surface format - channel order may not match ri_texture_format::R8G8B8A8_SRGB.");
    return { formats[0], formats[0].format, ri_texture_format::R8G8B8A8_SRGB, false };
}

}; // namespace

vulkan_ri_swapchain::vulkan_ri_swapchain(vulkan_render_interface& renderer, window& for_window, const char* debug_name)
    : m_renderer(renderer)
    , m_window(for_window)
    , m_debug_name(debug_name)
{
}

vulkan_ri_swapchain::~vulkan_ri_swapchain()
{
    destroy_resources();
}

void vulkan_ri_swapchain::destroy_resources()
{
    for (size_t i = 0; i < m_image_count; i++)
    {
        m_back_buffer_targets[i] = nullptr;
        m_back_buffer_last_used_frame[i] = 0;
    }
    m_image_count = 0;

    drain();

    if (m_acquire_fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(m_renderer.get_device(), m_acquire_fence, nullptr);
        m_acquire_fence = VK_NULL_HANDLE;
    }

    if (m_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_renderer.get_device(), m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_renderer.get_instance(), m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
}

result<void> vulkan_ri_swapchain::create_resources()
{
    memory_scope mem_scope(memory_type::rendering__vram__swapchain, string_hash::empty, string_hash(m_debug_name));

    m_window_width = m_window.get_width();
    m_window_height = m_window.get_height();
    m_window_mode = m_window.get_mode();

    SDL_Window* sdl_window_handle = reinterpret_cast<SDL_Window*>(m_window.get_platform_handle());
    if (!SDL_Vulkan_CreateSurface(sdl_window_handle, m_renderer.get_instance(), &m_surface))
    {
        db_error(render_interface, "SDL_Vulkan_CreateSurface failed with error: %s", SDL_GetError());
        return standard_errors::failed;
    }

    chosen_swapchain_format chosen_format = choose_surface_format(m_renderer, m_surface);
    VkSurfaceFormatKHR surface_format = chosen_format.surface_format;
    m_format = surface_format.format;
    m_view_format = chosen_format.view_format;
    m_ri_format = chosen_format.ri_format;
    m_needs_mutable_format = chosen_format.needs_mutable_format;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_renderer.get_physical_device(), m_surface, &capabilities);

    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == std::numeric_limits<uint32_t>::max())
    {
        extent.width = std::clamp(static_cast<uint32_t>(m_window_width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(static_cast<uint32_t>(m_window_height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32_t min_image_count = std::max(static_cast<uint32_t>(k_buffer_count), capabilities.minImageCount);
    if (capabilities.maxImageCount > 0)
    {
        min_image_count = std::min(min_image_count, capabilities.maxImageCount);
    }
    min_image_count = std::min(min_image_count, static_cast<uint32_t>(k_max_buffer_count));

    // Backbuffers are wrapped as regular vulkan_ri_texture instances, which unconditionally
    // register a bindless sampled-image SRV for every texture - so the backbuffer image itself
    // needs VK_IMAGE_USAGE_SAMPLED_BIT or that SRV creation fails validation.
    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        image_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    std::array<VkFormat, 2> mutable_view_formats = { m_format, m_view_format };

    VkImageFormatListCreateInfo format_list_create_info = {};
    format_list_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
    format_list_create_info.viewFormatCount = static_cast<uint32_t>(mutable_view_formats.size());
    format_list_create_info.pViewFormats = mutable_view_formats.data();

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = m_surface;
    swapchain_create_info.minImageCount = min_image_count;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = image_usage;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.preTransform = capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
    if (m_needs_mutable_format)
    {
        swapchain_create_info.flags |= VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
        swapchain_create_info.pNext = &format_list_create_info;
    }

    VkResult vk_result = vkCreateSwapchainKHR(m_renderer.get_device(), &swapchain_create_info, nullptr, &m_swapchain);
    if (!m_renderer.check_result(vk_result, "vkCreateSwapchainKHR"))
    {
        return standard_errors::failed;
    }

    if (!create_render_targets())
    {
        return standard_errors::failed;
    }

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    vk_result = vkCreateFence(m_renderer.get_device(), &fence_create_info, nullptr, &m_acquire_fence);
    if (!m_renderer.check_result(vk_result, "vkCreateFence"))
    {
        return standard_errors::failed;
    }

    m_fence = m_renderer.create_fence(string_format("%s - Swap Chain Fence", m_debug_name.c_str()).c_str());
    if (!m_fence)
    {
        return standard_errors::failed;
    }

    return true;
}

result<void> vulkan_ri_swapchain::create_render_targets()
{
    memory_scope mem_scope(memory_type::rendering__vram__swapchain, string_hash::empty, string_hash(m_debug_name));

    uint32_t image_count = 0;
    vkGetSwapchainImagesKHR(m_renderer.get_device(), m_swapchain, &image_count, nullptr);

    std::vector<VkImage> images(image_count);
    vkGetSwapchainImagesKHR(m_renderer.get_device(), m_swapchain, &image_count, images.data());

    db_assert_message(image_count <= k_max_buffer_count, "Vulkan swapchain returned more images than expected.");

    m_image_count = std::min(static_cast<size_t>(image_count), k_max_buffer_count);

    for (size_t i = 0; i < m_image_count; i++)
    {
        ri_texture::create_params create_params;
        create_params.width = m_window_width;
        create_params.height = m_window_height;
        create_params.format = m_ri_format;
        create_params.is_render_target = true;

        std::string buffer_name = string_format("%s[%i]", m_debug_name.c_str(), i);
        m_back_buffer_last_used_frame[i] = 0;
        m_back_buffer_targets[i] = std::make_unique<vulkan_ri_texture>(
            m_renderer,
            buffer_name.c_str(),
            create_params,
            images[i],
            m_view_format,
            ri_resource_state::present);
    }

    m_memory_allocation_info = mem_scope.record_alloc(m_window_width * m_window_height * 4 * m_image_count);

    return true;
}

result<void> vulkan_ri_swapchain::resize_buffers()
{
    for (size_t i = 0; i < m_image_count; i++)
    {
        m_back_buffer_targets[i] = nullptr;
        m_back_buffer_last_used_frame[i] = 0;
    }
    m_image_count = 0;

    drain();

    VkSwapchainKHR old_swapchain = m_swapchain;

    m_window_width = m_window.get_width();
    m_window_height = m_window.get_height();
    m_window_mode = m_window.get_mode();

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_renderer.get_physical_device(), m_surface, &capabilities);

    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == std::numeric_limits<uint32_t>::max())
    {
        extent.width = std::clamp(static_cast<uint32_t>(m_window_width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(static_cast<uint32_t>(m_window_height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32_t min_image_count = std::max(static_cast<uint32_t>(k_buffer_count), capabilities.minImageCount);
    if (capabilities.maxImageCount > 0)
    {
        min_image_count = std::min(min_image_count, capabilities.maxImageCount);
    }
    min_image_count = std::min(min_image_count, static_cast<uint32_t>(k_max_buffer_count));

    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        image_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    std::array<VkFormat, 2> mutable_view_formats = { m_format, m_view_format };

    VkImageFormatListCreateInfo format_list_create_info = {};
    format_list_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
    format_list_create_info.viewFormatCount = static_cast<uint32_t>(mutable_view_formats.size());
    format_list_create_info.pViewFormats = mutable_view_formats.data();

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = m_surface;
    swapchain_create_info.minImageCount = min_image_count;
    swapchain_create_info.imageFormat = m_format;
    swapchain_create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_create_info.imageExtent = extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = image_usage;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.preTransform = capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = old_swapchain;
    if (m_needs_mutable_format)
    {
        swapchain_create_info.flags |= VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
        swapchain_create_info.pNext = &format_list_create_info;
    }

    VkResult vk_result = vkCreateSwapchainKHR(m_renderer.get_device(), &swapchain_create_info, nullptr, &m_swapchain);

    if (old_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_renderer.get_device(), old_swapchain, nullptr);
    }

    if (!m_renderer.check_result(vk_result, "vkCreateSwapchainKHR"))
    {
        return standard_errors::failed;
    }

    if (!create_render_targets())
    {
        return standard_errors::failed;
    }

    return true;
}

ri_texture& vulkan_ri_swapchain::next_backbuffer()
{
    vkResetFences(m_renderer.get_device(), 1, &m_acquire_fence);

    uint32_t image_index = 0;
    VkResult vk_result = vkAcquireNextImageKHR(m_renderer.get_device(), m_swapchain, UINT64_MAX, VK_NULL_HANDLE, m_acquire_fence, &image_index);
    if (vk_result != VK_SUBOPTIMAL_KHR)
    {
        m_renderer.assert_result(vk_result, "vkAcquireNextImageKHR");
    }

    vkWaitForFences(m_renderer.get_device(), 1, &m_acquire_fence, VK_TRUE, UINT64_MAX);

    m_current_buffer_index = image_index;

    size_t last_frame_used = m_back_buffer_last_used_frame[m_current_buffer_index];
    if (last_frame_used > 0)
    {
        profile_marker(profile_colors::wait, "wait for gpu");
        m_fence->wait(last_frame_used);
    }

    return *m_back_buffer_targets[m_current_buffer_index];
}

void vulkan_ri_swapchain::present()
{
    profile_marker(profile_colors::wait, "present");

    vulkan_ri_command_queue& graphics_queue = static_cast<vulkan_ri_command_queue&>(m_renderer.get_graphics_queue());

    // See class comment - no semaphore threaded through command_queue's submissions yet, so
    // wait for all rendering to finish before handing the image to the presentation engine.
    m_renderer.assert_result(vkQueueWaitIdle(graphics_queue.get_queue()), "vkQueueWaitIdle");

    uint32_t image_index = static_cast<uint32_t>(m_current_buffer_index);

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &m_swapchain;
    present_info.pImageIndices = &image_index;

    VkResult vk_result = vkQueuePresentKHR(graphics_queue.get_queue(), &present_info);
    if (vk_result != VK_SUBOPTIMAL_KHR)
    {
        m_renderer.assert_result(vk_result, "vkQueuePresentKHR");
    }

    m_back_buffer_last_used_frame[m_current_buffer_index] = m_frame_index;

    m_fence->signal(m_renderer.get_graphics_queue(), m_frame_index);

    m_frame_index++;

    if (m_window.get_width() != m_window_width ||
        m_window.get_height() != m_window_height ||
        m_window.get_mode() != m_window_mode)
    {
        db_log(render_interface, "Window metrics changed, recreating swapchain.");

        if (!resize_buffers())
        {
            db_fatal(render_interface, "Failed to recreate swapchain.");
        }
    }
}

void vulkan_ri_swapchain::drain()
{
    if (m_frame_index > 0 && m_fence)
    {
        profile_marker(profile_colors::wait, "draining gpu");
        m_fence->wait(m_frame_index - 1);
    }

    m_renderer.drain_deferred();
}

}; // namespace ws
