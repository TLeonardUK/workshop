// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.core/utils/result.h"
#include "workshop.core/containers/memory_heap.h"
#include "workshop.core/memory/memory_tracker.h"

#include <mutex>
#include <vector>
#include <memory>

namespace ws {

class vulkan_render_interface;
class vulkan_ri_texture;

// ================================================================================================
//  This class manages the creation and updating of tiles for reserved resources.
// ================================================================================================
class vulkan_ri_tile_manager
{
public:
    struct heap_state
    {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        std::unique_ptr<memory_heap> heap;
        size_t size_in_tiles = 0;

        std::unique_ptr<memory_allocation> slack_memory_allocation_info;
    };

    struct heap_tile_allocation
    {
        heap_state* heap = nullptr;
        size_t tile_offset = 0;
        size_t tile_count = 0;
    };

    struct allocation
    {
        bool is_valid = false;
        std::vector<heap_tile_allocation> heap_allocations;
    };

public:
    vulkan_ri_tile_manager(vulkan_render_interface& renderer);
    ~vulkan_ri_tile_manager();

    result<void> create_resources();

    void begin_frame();

    allocation allocate_tiles(size_t tile_count);
    void free_tiles(allocation alloc);

    // Queues a bind of the given tile allocation to a texture at the given mip, executed as
    // part of the next vkQueueBindSparse submission in begin_frame().
    void queue_map(vulkan_ri_texture& texture, allocation alloc, size_t mip_index);

    // Queues an unbind of whatever tiles are currently mapped to a texture at the given mip.
    void queue_unmap(vulkan_ri_texture& texture, size_t mip_index);

    // Byte size of one tile - see class comment for why this is a fixed assumption.
    static constexpr size_t k_tile_byte_size = 64 * 1024;

private:
    enum class operation_type
    {
        free_tiles,
        unmap,
        map,
    };

    struct operation
    {
        operation_type type;

        size_t frame_index = 0;

        allocation alloc;

        vulkan_ri_texture* texture = nullptr;
        VkImage image = VK_NULL_HANDLE;
        size_t mip_index = 0;

        bool is_packed = false;
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        VkExtent3D pixel_extent = {};

        // Byte offset/size of the packed mip tail within the image's opaque memory layout,
        // only valid when is_packed is true (queried once via VkSparseImageMemoryRequirements).
        VkDeviceSize mip_tail_offset = 0;
        VkDeviceSize mip_tail_size = 0;
    };

    // Granularity of heap size in tiles.
    static constexpr size_t k_heap_granularity_in_tiles = 4096; // 256mb @ 64kb/tile

private:
    void allocate_new_heap(size_t minimum_size_in_tiles);

    void perform_operations(size_t frame_index);
    void submit_bind_sparse(const std::vector<operation*>& ops);

private:
    std::recursive_mutex m_mutex;
    std::vector<operation> m_operations;

    vulkan_render_interface& m_renderer;

    size_t m_frame_index = 0;

    std::vector<std::unique_ptr<heap_state>> m_heaps;

    VkFence m_bind_sparse_fence = VK_NULL_HANDLE;

};

}; // namespace ws
