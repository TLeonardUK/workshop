// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.render_interface/ri_types.h"

#include "workshop.core/containers/memory_heap.h"

#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace ws {

class vulkan_render_interface;
class vulkan_ri_command_list;
class vulkan_ri_texture;
class vulkan_ri_buffer;
class vulkan_ri_staging_buffer;
class statistics_channel;

// ================================================================================================
//  Handles copying CPU data to GPU memory.
// ================================================================================================
class vulkan_ri_upload_manager
{
public:
    using build_command_list_callback_t = std::function<void(vulkan_ri_command_list&)>;

    struct heap_state
    {
        VkBuffer handle = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        uint8_t* start_ptr = nullptr;

        std::unique_ptr<memory_heap> heap;
        size_t size = 0;

        size_t last_allocation_frame = 0;
    };

    struct upload_state
    {
        size_t freed_frame_index = 0;
        size_t queued_frame_index = 0;
        size_t heap_offset = 0;
        size_t heap_size = 0;

        heap_state* heap = nullptr;

        // Only one of image/buffer is set, depending on which upload() overload queued this.
        VkImage image = VK_NULL_HANDLE;
        VkImageAspectFlags image_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        VkBuffer buffer = VK_NULL_HANDLE;

        // Source layout for the pre-copy transition into copy_dest - may legitimately be
        // ri_resource_state::initial (VK_IMAGE_LAYOUT_UNDEFINED) for a texture's first-ever
        // use, unlike resource_steady_state below.
        ri_resource_state resource_initial_state = ri_resource_state::initial;

        // Destination layout for the post-copy transition back out of copy_dest - always the
        // texture's real resolved common state, never ri_resource_state::initial, since that
        // would leave the image in VK_IMAGE_LAYOUT_UNDEFINED after the upload completes.
        ri_resource_state resource_steady_state = ri_resource_state::initial;

        build_command_list_callback_t build_command_list = nullptr;

        const char* name = nullptr;
    };

    // A pending image layout transition with no associated data copy (eg. a freshly created
    // render target that just needs to leave VK_IMAGE_LAYOUT_UNDEFINED before anything can
    // reference it). Queued the same way as a real upload - cheaply, from any thread - and
    // flushed into a single batched command list alongside the rest of flush_uploads(), rather
    // than needing its own dedicated command pool and a blocking wait per texture.
    struct transition_state
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageAspectFlags image_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        ri_resource_state source_state = ri_resource_state::initial;
        ri_resource_state destination_state = ri_resource_state::initial;
    };

public:
    vulkan_ri_upload_manager(vulkan_render_interface& renderer);
    ~vulkan_ri_upload_manager();

    result<void> create_resources();

    void begin_frame();
    void flush();

    upload_state allocate_upload(size_t size, size_t alignment);
    void queue_upload(upload_state state);
    void queue_transition(VkImage image, VkImageAspectFlags aspect, ri_resource_state source_state, ri_resource_state destination_state);

    void upload(vulkan_ri_texture& texture, std::span<uint8_t> data);
    void upload(vulkan_ri_texture& texture, size_t array_index, size_t mip_index, std::span<uint8_t> data);
    void upload(vulkan_ri_texture& texture, size_t array_index, size_t mip_index, vulkan_ri_staging_buffer& data_buffer);
    void upload(vulkan_ri_buffer& buffer, std::span<uint8_t> data, size_t offset);

private:
    friend class vulkan_ri_staging_buffer;

    void allocate_new_heap(size_t minimum_size);

    void perform_transitions();
    void perform_uploads();
    void free_uploads();

    size_t get_heap_size();

    // Granularity of heap size. The actual heap size is based on the size of the data to
    // be uploaded.
    static constexpr size_t k_heap_granularity = 32 * 1024 * 1024;

    // Don't deallocate memory if all heaps are below this size.
    static constexpr size_t k_persist_heap_memory = 256 * 1024 * 1024;

private:
    vulkan_render_interface& m_renderer;

    std::recursive_mutex m_pending_upload_mutex;
    std::vector<upload_state> m_pending_uploads;
    std::vector<upload_state> m_pending_free;
    std::vector<transition_state> m_pending_transitions;

    size_t m_frame_index = 0;

    std::vector<std::unique_ptr<heap_state>> m_heaps;

    statistics_channel* m_stats_render_bytes_uploaded = nullptr;

};

}; // namespace ws
