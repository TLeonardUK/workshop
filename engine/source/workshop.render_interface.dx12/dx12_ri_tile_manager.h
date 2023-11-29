// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/containers/memory_heap.h"
#include "workshop.core/memory/memory_tracker.h"

#include "workshop.render_interface.dx12/dx12_headers.h"
#include "workshop.render_interface/ri_types.h"

#include <array>
#include <span>
#include <functional>

namespace ws {

class engine;
class dx12_render_interface;
class dx12_ri_texture;
class dx12_ri_buffer;
class dx12_ri_command_list;
class statistics_channel;
class ri_fence;

// ================================================================================================
//  This class manages the creation and updating of tiles for reserved resources.
// ================================================================================================
class dx12_ri_tile_manager
{
public:
    struct heap_tile_allocation
    {
        ID3D12Heap* heap;
        size_t tile_offset;
        size_t tile_count;
    };

    struct tile_allocation
    {
        std::vector<heap_tile_allocation> heap_allocations;
    };

public:
    dx12_ri_tile_manager(dx12_render_interface& renderer);
    virtual ~dx12_ri_tile_manager();

    result<void> create_resources();

    tile_allocation allocate_tiles(size_t count);
    void free_tiles(tile_allocation allocation);

    void queue_map(dx12_ri_texture& texture, tile_allocation allocation, size_t mip_index);
    void queue_unmap(dx12_ri_texture& texture, size_t mip_index);

    void new_frame(size_t index);

private:
    using build_command_list_callback_t = std::function<void(dx12_ri_command_list& list)>;

    struct heap_state
    {
        Microsoft::WRL::ComPtr<ID3D12Heap> handle = nullptr;
        uint8_t* start_ptr = nullptr;

        std::unique_ptr<memory_heap> memory_heap;
        size_t size_in_tiles = 0;

        std::unique_ptr<memory_allocation> slack_memory_allocation_info;
    };

    // Granularity of heap size in tiles. Each tile is typically 64kb.
    static constexpr size_t k_heap_granularity_in_tiles = 128;

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

        // Frame index to run operation on.
        size_t frame_index;

        tile_allocation allocation;

        dx12_ri_texture* texture;
        size_t mip_index;
    };

private:
    void allocate_new_heap(size_t minimum_size_in_tiles);
    void free_heap(heap_state* heap);
    heap_state* get_heap_state(ID3D12Heap* heap);

    void perform_operation(operation& op);
    void perform_operations(size_t frame_index);

private:
    std::recursive_mutex m_mutex;
    std::vector<operation> m_operations;
    
    dx12_render_interface& m_renderer;

    size_t m_frame_index = 0;

    std::vector<std::unique_ptr<heap_state>> m_heaps;

};

}; // namespace ws
