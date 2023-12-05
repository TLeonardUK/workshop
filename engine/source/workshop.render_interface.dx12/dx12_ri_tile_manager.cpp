// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_tile_manager.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_fence.h"
#include "workshop.render_interface.dx12/dx12_ri_texture.h"
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"
#include "workshop.core/statistics/statistics_manager.h"
#include "workshop.core/utils/time.h"

// If set whenever a tile heap is empty it will be deallocate.
// This reduces memory usage but can lead to spikes if heaps need to be reallocate due to texture streaming.
// The tile memory usage is generally handled higher by the texture streaming pool size, so allocating up
// to the max and persisting at it should be fine.
//#define DEALLOCATE_EMPTY_TILE_HEAPS 1

namespace ws {

dx12_ri_tile_manager::dx12_ri_tile_manager(dx12_render_interface& renderer)
    : m_renderer(renderer)
{
}

dx12_ri_tile_manager::~dx12_ri_tile_manager()
{
    m_heaps.clear();
}

void dx12_ri_tile_manager::allocate_new_heap(size_t minimum_size_in_tiles)
{
    memory_scope mem_scope(memory_type::rendering__vram__tile_heap, memory_scope::k_ignore_asset);

    std::unique_ptr<heap_state> state = std::make_unique<heap_state>();
    state->size_in_tiles = math::round_up_multiple(minimum_size_in_tiles, k_heap_granularity_in_tiles);

    D3D12_HEAP_PROPERTIES heap_properties;
    heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_properties.CreationNodeMask = 0;
    heap_properties.VisibleNodeMask = 0;

    D3D12_HEAP_DESC heap_desc;
    heap_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    heap_desc.Flags = D3D12_HEAP_FLAG_NONE;
    heap_desc.Properties = heap_properties;
    heap_desc.SizeInBytes = state->size_in_tiles * D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    HRESULT hr = m_renderer.get_device()->CreateHeap(
        &heap_desc,
        IID_PPV_ARGS(&state->handle)
    );
    if (FAILED(hr))
    {
        db_fatal(render_interface, "CreateHeap failed with error 0x%08x when creating upload heap.", hr);
    }

    // Record the memory allocation.
    state->memory_heap = std::make_unique<memory_heap>(state->size_in_tiles);

    state->slack_memory_allocation_info = mem_scope.record_alloc(state->memory_heap->get_remaining() * D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    m_heaps.push_back(std::move(state));
}

void dx12_ri_tile_manager::free_heap(heap_state* heap)
{
    auto iter = std::find_if(m_heaps.begin(), m_heaps.end(), [heap](const auto& state) {
        return heap == state.get();
    });
    db_assert(iter != m_heaps.end());

    m_heaps.erase(iter);
}

dx12_ri_tile_manager::heap_state* dx12_ri_tile_manager::get_heap_state(ID3D12Heap* heap)
{
    for (auto& state : m_heaps)
    {
        if (state->handle.Get() == heap)
        {
            return state.get();
        }
    }
    return nullptr;
}

result<void> dx12_ri_tile_manager::create_resources()
{
    return true;
}

dx12_ri_tile_manager::tile_allocation dx12_ri_tile_manager::allocate_tiles(size_t count)
{
    profile_marker(profile_colors::render, "dx12_ri_tile_manager::allocate_tiles");

    std::scoped_lock lock(m_mutex);

    memory_scope mem_scope(memory_type::rendering__vram__tile_heap, memory_scope::k_ignore_asset);

    dx12_ri_tile_manager::tile_allocation allocation;
    size_t tiles_remaining = count;

    while (tiles_remaining > 0)
    {
        for (auto& heap : m_heaps)
        {
           heap_tile_allocation heap_allocation;
           heap_allocation.tile_count = tiles_remaining;
           heap_allocation.heap = heap->handle.Get();

            if (heap->memory_heap->alloc(heap_allocation.tile_count, 1, heap_allocation.tile_offset))
            {
                allocation.heap_allocations.push_back(heap_allocation);
                tiles_remaining -= heap_allocation.tile_count;

                // Update the heaps slack allocation.
                heap->slack_memory_allocation_info = mem_scope.record_alloc(heap->memory_heap->get_remaining() * D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

                break;
            }
        }

        if (tiles_remaining > 0)
        {
            db_log(core, "Allocating new tile heap.");
            allocate_new_heap(tiles_remaining);
        }
        else
        {
            break;
        }
    }

    return allocation;
}

void dx12_ri_tile_manager::free_tiles(tile_allocation allocation)
{
    profile_marker(profile_colors::render, "dx12_ri_tile_manager::free_tiles");

    std::scoped_lock lock(m_mutex);

    operation& op = m_operations.emplace_back();
    op.type = operation_type::free_tiles;
    op.allocation = allocation;
    // Free only after pipeline depth has elapsed so we can be assured the tiles are no longer in use.
    op.frame_index = m_frame_index + m_renderer.get_pipeline_depth();
}

void dx12_ri_tile_manager::queue_map(dx12_ri_texture& texture, tile_allocation allocation, size_t mip_index)
{
    profile_marker(profile_colors::render, "dx12_ri_tile_manager::queue_map");

    std::scoped_lock lock(m_mutex);

    // Remove any pending maps or unmaps from the operation queue to keep things coherent.
    auto iter = std::find_if(m_operations.begin(), m_operations.end(), [&texture, &mip_index](const operation& op) {
        return (op.mip_index == mip_index && op.texture == &texture) && (op.type == operation_type::map || op.type == operation_type::unmap);
    });
    if (iter != m_operations.end())
    {
        m_operations.erase(iter);
    }

    operation& op = m_operations.emplace_back();
    op.type = operation_type::map;
    op.allocation = allocation;
    op.texture = &texture;
    op.mip_index = mip_index;
    // We can map immediately.
    op.frame_index = m_frame_index;
}

void dx12_ri_tile_manager::queue_unmap(dx12_ri_texture& texture, size_t mip_index)
{
    profile_marker(profile_colors::render, "dx12_ri_tile_manager::queue_unmap");

    std::scoped_lock lock(m_mutex);

    // Remove any pending maps or unmaps from the operation queue to keep things coherent.
    auto iter = std::find_if(m_operations.begin(), m_operations.end(), [&texture, &mip_index](const operation& op) {
        return (op.mip_index == mip_index && op.texture == &texture) && (op.type == operation_type::map || op.type == operation_type::unmap);
    });
    if (iter != m_operations.end())
    {
        m_operations.erase(iter);
    }

    operation& op = m_operations.emplace_back();
    op.type = operation_type::unmap;
    op.texture = &texture;
    op.mip_index = mip_index;
    // Free only after pipeline depth has elapsed so we can be assured the tiles are no longer in use.
    op.frame_index = m_frame_index + m_renderer.get_pipeline_depth();
}

void dx12_ri_tile_manager::new_frame(size_t index)
{
    profile_marker(profile_colors::render, "dx12_ri_tile_manager::new_frame");

    memory_scope mem_scope(memory_type::rendering__tile_heap, memory_scope::k_ignore_asset);

    std::scoped_lock lock(m_mutex);

    perform_operations(index);

    m_frame_index = index;
}

void dx12_ri_tile_manager::perform_operation(operation& op)
{
    dx12_ri_command_queue& queue = static_cast<dx12_ri_command_queue&>(m_renderer.get_graphics_queue());

    switch (op.type)
    {
    case operation_type::free_tiles:
        {
            for (heap_tile_allocation& allocation : op.allocation.heap_allocations)
            {
                heap_state* heap = get_heap_state(allocation.heap);
                db_assert(heap);

                heap->memory_heap->free(allocation.tile_offset);

                // Update the heaps slack allocation.
                memory_scope mem_scope(memory_type::rendering__vram__tile_heap, memory_scope::k_ignore_asset);
                heap->slack_memory_allocation_info = mem_scope.record_alloc(heap->memory_heap->get_remaining() * D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

#ifdef DEALLOCATE_EMPTY_TILE_HEAPS
                if (heap->memory_heap->empty())
                {
                    db_log(core, "Freeing tile heap.");
                    free_heap(heap);
                }
#endif
            }
            break;
        }
    case operation_type::map:
        {
            for (heap_tile_allocation& alloc : op.allocation.heap_allocations)
            {
                const dx12_ri_texture::mip_residency& residency = op.texture->get_mip_residency(op.mip_index);

                D3D12_TILED_RESOURCE_COORDINATE resource_range_coordinate = residency.tile_coordinate;
                D3D12_TILE_REGION_SIZE resource_range_size = residency.tile_size;

                D3D12_TILE_RANGE_FLAGS range_flags = D3D12_TILE_RANGE_FLAG_NONE;
                UINT range_offset = (UINT)alloc.tile_offset;
                UINT range_size = (UINT)alloc.tile_count;

                queue.get_queue()->UpdateTileMappings(
                    op.texture->get_resource(),
                    1,
                    &resource_range_coordinate,
                    &resource_range_size,
                    alloc.heap,
                    1,
                    &range_flags,
                    &range_offset,
                    &range_size,
                    D3D12_TILE_MAPPING_FLAG_NONE
                );
            }
            break;
        }
    case operation_type::unmap:
        {
            for (heap_tile_allocation& alloc : op.allocation.heap_allocations)
            {
                const dx12_ri_texture::mip_residency& residency = op.texture->get_mip_residency(op.mip_index);

                D3D12_TILED_RESOURCE_COORDINATE resource_range_coordinate = residency.tile_coordinate;
                D3D12_TILE_REGION_SIZE resource_range_size = residency.tile_size;

                queue.get_queue()->UpdateTileMappings(
                    op.texture->get_resource(),
                    1,
                    &resource_range_coordinate,
                    &resource_range_size,
                    nullptr,
                    0,
                    nullptr,
                    nullptr,
                    nullptr,
                    D3D12_TILE_MAPPING_FLAG_NONE
                );
            }
            break;
        }
    }
}

void dx12_ri_tile_manager::perform_operations(size_t frame_index)
{
    for (auto iter = m_operations.begin(); iter != m_operations.end(); /* empty */)
    {
        operation& op = *iter;
        if (op.frame_index <= frame_index)
        {
            perform_operation(op);
            iter = m_operations.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

}; // namespace ws
