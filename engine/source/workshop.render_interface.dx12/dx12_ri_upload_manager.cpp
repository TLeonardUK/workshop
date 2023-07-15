// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_upload_manager.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_fence.h"
#include "workshop.render_interface.dx12/dx12_ri_texture.h"
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"
#include "workshop.core/statistics/statistics_manager.h"

namespace ws {

dx12_ri_upload_manager::dx12_ri_upload_manager(dx12_render_interface& renderer)
    : m_renderer(renderer)
{
    m_stats_render_bytes_uploaded = statistics_manager::get().find_or_create_channel("render/bytes uploaded", 1.0, statistics_commit_point::end_of_render);
}

dx12_ri_upload_manager::~dx12_ri_upload_manager()
{
    for (auto& state : m_heaps)
    {
        D3D12_RANGE range;
        range.Begin = 0;
        range.End = k_heap_size;
        state->handle->Unmap(0, &range);
        state->handle = nullptr;

        //CheckedRelease(state->handle);
    }
    m_heaps.clear();

    m_graphics_queue_fence = nullptr;
    m_copy_queue_fence = nullptr;
}

void dx12_ri_upload_manager::allocate_new_heap()
{
    std::unique_ptr<heap_state> state = std::make_unique<heap_state>();

    D3D12_HEAP_PROPERTIES heap_properties;
    heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_properties.CreationNodeMask = 0;
    heap_properties.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC desc;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.DepthOrArraySize = 1;
    desc.Width = k_heap_size;
    desc.Height = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    HRESULT hr = m_renderer.get_device()->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&state->handle)
    );
    if (FAILED(hr))
    {
        db_fatal(render_interface, "CreateCommittedResource failed with error 0x%08x when creating upload heap.", hr);
    }

    D3D12_RANGE range;
    range.Begin = 0;
    range.End = k_heap_size;

    hr = state->handle->Map(0, &range, (void**)&state->start_ptr);
    if (FAILED(hr))
    {
        db_fatal(render_interface, "Mapping upload heap failed with error 0x%08x.", hr);
    }

    state->memory_heap = std::make_unique<memory_heap>(k_heap_size);

    m_heaps.push_back(std::move(state));
}

result<void> dx12_ri_upload_manager::create_resources()
{
    m_graphics_queue_fence = m_renderer.create_fence("Upload Manager - Graphics Fence");
    m_copy_queue_fence = m_renderer.create_fence("Upload Manager - Copy Fence");
    if (!m_graphics_queue_fence || !m_copy_queue_fence)
    {
        return false;
    }

    allocate_new_heap();

    return true;
}

void dx12_ri_upload_manager::upload(dx12_ri_texture& source, const std::span<uint8_t>& data)
{
    ri_command_queue& queue = m_renderer.get_copy_queue();

    // Calculate the offsets of each face in the stored data.
    size_t block_size = ri_format_block_size(source.get_format());
    size_t face_count = source.get_depth();
    size_t slice_count = source.get_depth();
    size_t mip_count = source.get_mip_levels();
    size_t array_count = 1;
    if (source.get_dimensions() == ri_texture_dimension::texture_cube)
    {
        face_count = 6;
        array_count = 6;
        slice_count = 1;
    }

    size_t sub_resource_count = mip_count * array_count;
    uint64_t total_memory = 0;

    std::vector<UINT> row_count;
    row_count.resize(sub_resource_count);

    std::vector<UINT64> row_size;
    row_size.resize(sub_resource_count);

    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints;
    footprints.resize(sub_resource_count);

    D3D12_RESOURCE_DESC desc = source.get_resource()->GetDesc();

    m_renderer.get_device()->GetCopyableFootprints(
        &desc,
        0u,
        static_cast<UINT>(sub_resource_count),
        0u,
        footprints.data(),
        row_count.data(),
        row_size.data(),
        &total_memory
    );

    upload_state upload = allocate_upload(total_memory, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    upload.resource = source.get_resource();
    upload.resource_initial_state = source.get_initial_state();

    // Zero out upload space as parts of it may be padding.
    memset(upload.heap->start_ptr + upload.heap_offset, 0, upload.heap_size);

    // TODO: All the below is pretty grim, this needs cleaning up and simplifying.

    // Copy source data into copy data. Source data is expected to be tightly packed
    // and stored linearly.
    uint8_t* source_data = data.data();

    std::vector<uint8_t*> face_mip_data;
    face_mip_data.resize(face_count * mip_count);

    size_t data_offset = 0;
    for (size_t face = 0; face < face_count; face++)
    {   
        size_t mip_width  = source.get_width();
        size_t mip_height = source.get_height();

        for (size_t mip = 0; mip < source.get_mip_levels(); mip++)
        {
            const size_t mip_index = mip + (face * mip_count);
            const size_t mip_size = (ri_bytes_per_texel(source.get_format()) * mip_width * mip_height) / block_size;
            face_mip_data[mip_index] = source_data + data_offset;

            data_offset += mip_size;

            mip_width = std::max(1llu, mip_width / 2);
            mip_height = std::max(1llu, mip_height / 2);
        }
    }

    // Copy the source data into each subresource.
    for (size_t array_index = 0; array_index < array_count; array_index++)
    {
        for (size_t mip_index = 0; mip_index < mip_count; mip_index++)
        {
            const size_t sub_resource_index = mip_index + (array_index * mip_count);

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint = footprints[sub_resource_index];
            size_t height = row_count[sub_resource_index];
            size_t pitch = math::round_up_multiple((size_t)footprint.Footprint.RowPitch, (size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
            size_t depth = footprint.Footprint.Depth;

            uint8_t* destination = (upload.heap->start_ptr + upload.heap_offset) + footprint.Offset;

            for (size_t slice_index = 0; slice_index < depth; slice_index++)
            {
                // We don't support 3d texture arrays yet so either slice_index or array_index will be 0.
                const size_t source_face_index = (array_index + slice_index);
                const size_t source_mip_index = (source_face_index * mip_count) + mip_index;
                uint8_t* source_data = face_mip_data[source_mip_index];
                size_t source_data_offset = 0;

                // Copy source data into each row.
                size_t dest_offset = 0;
                for (size_t height_index = 0; height_index < height; height_index++)
                {
                    size_t source_row_size = footprint.Footprint.Width * ri_bytes_per_texel(source.get_format());
                    db_assert(source_row_size == row_size[sub_resource_index]);

#ifdef UPLOAD_DEBUG
                    if (footprint.Offset + dest_offset + source_row_size > upload.heap_size)
                    {
                        __debugbreak();
                    }
#endif

                    memcpy(destination, source_data + source_data_offset, source_row_size);

                    source_data_offset += source_row_size;
                    destination += pitch;
                    dest_offset += pitch;
                }
            }
        }
    }

    // Generate copy command list.
    upload.build_command_list = [sub_resource_count, source_ptr=source.get_resource(), footprints, heap_ptr=upload.heap->handle.Get(), heap_offset=upload.heap_offset](dx12_ri_command_list& list)
    {
        for (size_t i = 0; i < sub_resource_count; i++)
        {
            D3D12_TEXTURE_COPY_LOCATION dest = {};
            dest.pResource = source_ptr;
            dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dest.SubresourceIndex = static_cast<UINT>(i);

            D3D12_TEXTURE_COPY_LOCATION src = {};
            src.pResource = heap_ptr;
            src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            src.PlacedFootprint = footprints[i];
            src.PlacedFootprint.Offset += heap_offset;

            list.get_dx_command_list()->CopyTextureRegion(
                &dest,
                0,
                0,
                0,
                &src,
                nullptr
            );
        }
    };
    
    queue_upload(upload);
}

void dx12_ri_upload_manager::upload(dx12_ri_buffer& source, const std::span<uint8_t>& data, size_t offset)
{
    ri_command_queue& queue = m_renderer.get_copy_queue();

    upload_state upload = allocate_upload(data.size(), source.get_element_size());
    upload.resource = source.get_resource();
    upload.resource_initial_state = source.get_initial_state();

    memcpy(upload.heap->start_ptr + upload.heap_offset, data.data(), data.size());

#ifdef UPLOAD_DEBUG
    std::vector<uint8_t> debug_copy = std::vector<uint8_t>(data.begin(), data.end());
    db_log(renderer, "Allocated: %p (%zi)", upload.heap->start_ptr + upload.heap_offset, upload.heap_size);
    upload.build_command_list = [source_ptr = source.get_resource(), start_ptr = upload.heap->start_ptr, debug_data = debug_copy, heap_ptr = upload.heap->handle.Get(), heap_offset = upload.heap_offset, size = data.size(), offset](dx12_ri_command_list& list)
#else
    upload.build_command_list = [source_ptr = source.get_resource(), heap_ptr = upload.heap->handle.Get(), heap_offset = upload.heap_offset, size = data.size(), offset](dx12_ri_command_list& list)
#endif
    {
#ifdef UPLOAD_DEBUG
        db_log(renderer, "Uploading: %p (%zi)", start_ptr + heap_offset, size);
        if (memcmp(start_ptr + heap_offset, debug_data.data(), size) != 0)
        {
            db_log(renderer, "Failed: %p (%zi)", start_ptr + heap_offset, size);
            __debugbreak();
        }
#endif

        list.get_dx_command_list()->CopyBufferRegion(
            source_ptr, 
            offset, 
            heap_ptr, 
            heap_offset, 
            size
        );
    };

    queue_upload(upload);
}

dx12_ri_upload_manager::upload_state dx12_ri_upload_manager::allocate_upload(size_t size, size_t alignment)
{
    db_assert_message(size < k_heap_size, "Upload is larger than entire heap size. Increase the heap size if you need to upload this much data.");

    std::scoped_lock lock(m_pending_upload_mutex);

    upload_state state;

    while (true)
    {
        for (auto& heap : m_heaps)
        {
            if (heap->memory_heap->alloc(size, alignment, state.heap_offset))
            {
                heap->last_allocation_frame = m_frame_index;

                state.heap_size = size;
                state.heap = heap.get();
                break;
            }
        }

        if (!state.heap)
        {
            allocate_new_heap();
        }
        else
        {
            break;
        }
    }

    return state;
}

void dx12_ri_upload_manager::queue_upload(upload_state state)
{
    std::scoped_lock lock(m_pending_upload_mutex);
    state.queued_frame_index = m_frame_index;
    m_pending_uploads.push_back(state);
}

void dx12_ri_upload_manager::new_frame(size_t index)
{
    std::scoped_lock lock(m_pending_upload_mutex);

    perform_uploads();
    free_uploads();

    m_frame_index = index;
}

void dx12_ri_upload_manager::free_uploads()
{
    profile_marker(profile_colors::render, "free uploads");

    size_t pipeline_depth = m_renderer.get_pipeline_depth();
    if (m_frame_index < pipeline_depth)
    {
        return;
    }

    size_t free_frame_index = (m_frame_index - pipeline_depth);

    for (auto iter = m_pending_free.begin(); iter != m_pending_free.end(); /* empty */)
    {
        if (iter->freed_frame_index <= free_frame_index)
        {
            profile_marker(profile_colors::render, "free allocation");

#ifdef UPLOAD_DEBUG
            memset(iter->heap->start_ptr + iter->heap_offset, 0, iter->heap_size);
            db_log(renderer, "Freeing: %p (%zi)", iter->heap->start_ptr + iter->heap_offset, iter->heap_size);
#endif

            iter->heap->memory_heap->free(iter->heap_offset);
            iter = m_pending_free.erase(iter);
        }
        else
        {
            iter++;
        }
    }

    // Nuke any heaps (apart from first) that haven't contained uploads in the full frame depth.
    if (m_heaps.size() > 1)
    {
        for (size_t i = 1; i < m_heaps.size(); /* empty */)
        {
            heap_state* heap = m_heaps[i].get();

            if (heap->memory_heap->empty())
            {
                profile_marker(profile_colors::render, "free heap");

                D3D12_RANGE range;
                range.Begin = 0;
                range.End = k_heap_size;
                heap->handle->Unmap(0, &range);

                //CheckedRelease(heap->handle);
                heap->handle = nullptr;

                m_heaps.erase(m_heaps.begin() + i);
            }
            else
            {
                i++;
            }
        }
    }
}
void dx12_ri_upload_manager::perform_uploads()
{
    profile_marker(profile_colors::render, "perform uploads");

    ri_command_queue& copy_queue = m_renderer.get_copy_queue();
    ri_command_queue& graphics_queue = m_renderer.get_graphics_queue();

    size_t fence_value = m_frame_index + 1;

    size_t total_bytes = 0;

    std::vector<upload_state> uploads = m_pending_uploads;
    m_pending_uploads.clear();

    if (uploads.empty())
    {
        m_stats_render_bytes_uploaded->submit(0.0);
        return;
    }

    // Seperate out all the unique resources for transitions. We have to do this
    // as we can perform multiple uploads to the same resource.
    std::vector<upload_state> unique_resources;
    for (upload_state& upload : uploads)
    {
        auto iter = std::find_if(unique_resources.begin(), unique_resources.end(), [&upload](upload_state& b) {
            return b.resource == upload.resource;
        });
        if (iter == unique_resources.end())
        {
            unique_resources.push_back(upload);
        }
    }

    profile_gpu_marker(graphics_queue, profile_colors::gpu_transition, "uploads");
    profile_gpu_marker(copy_queue, profile_colors::gpu_transition, "uploads");

    // Transition all pending resources to common on graphics queue.
    {
        dx12_ri_command_list& transition_list = static_cast<dx12_ri_command_list&>(graphics_queue.alloc_command_list());
        transition_list.open();
        for (upload_state& state : unique_resources)
        {
            transition_list.barrier(state.resource, state.resource_initial_state, ri_resource_state::initial, ri_resource_state::common_state);
        }        
        transition_list.close();

        profile_gpu_marker(graphics_queue, profile_colors::gpu_transition, "transition resources to common");
        graphics_queue.execute(transition_list);
    }

    // Signal copy queue on graphics queue when we have finished transitioning.
    m_graphics_queue_fence->signal(graphics_queue, fence_value);

    // Wait on graphics queue for copy queue to finish copying.
    {
        profile_gpu_marker(graphics_queue, profile_colors::gpu_transition, "wait for uploads to complete");
        m_copy_queue_fence->wait(graphics_queue, fence_value);
    }

    // Wait on copy queue for graphics queue to finish transitioning.
    {
        profile_gpu_marker(graphics_queue, profile_colors::gpu_transition, "wait for transitions");
        m_graphics_queue_fence->wait(copy_queue, fence_value);
    }

    // Perform actual copies on the copy queue.
    {
        profile_gpu_marker(graphics_queue, profile_colors::gpu_transition, "upload resources");

        // Transition from common to a copy destination.
        {
            dx12_ri_command_list& transition_list = static_cast<dx12_ri_command_list&>(copy_queue.alloc_command_list());
            transition_list.open();
            for (upload_state& state : unique_resources)
            {
                transition_list.barrier(state.resource, state.resource_initial_state, ri_resource_state::common_state, ri_resource_state::copy_dest);
            }
            transition_list.close();

            profile_gpu_marker(copy_queue, profile_colors::gpu_transition, "transition resources to copy destination");
            copy_queue.execute(transition_list);
        }

        // Perform the actual copies.
        dx12_ri_command_list& list = static_cast<dx12_ri_command_list&>(copy_queue.alloc_command_list());
        list.open();
        for (upload_state& state : uploads)
        {
            state.build_command_list(list);

            state.freed_frame_index = m_frame_index;
            m_pending_free.push_back(state);

            total_bytes += state.heap_size;
        }
        list.close();
        copy_queue.execute(list);

        // Transition back from copy destination to common.
        {
            dx12_ri_command_list& transition_list = static_cast<dx12_ri_command_list&>(copy_queue.alloc_command_list());
            transition_list.open();
            for (upload_state& state : unique_resources)
            {
                transition_list.barrier(state.resource, state.resource_initial_state, ri_resource_state::copy_dest, ri_resource_state::common_state);
            }
            transition_list.close();

            profile_gpu_marker(copy_queue, profile_colors::gpu_transition, "transition resources to copy destination");
            copy_queue.execute(transition_list);
        }
    }

    // Signal graphics queue that we have finished.
    m_copy_queue_fence->signal(copy_queue, fence_value);

    // Graphics queue now transitions everything back from common to initial state.
    {
        dx12_ri_command_list& transition_list = static_cast<dx12_ri_command_list&>(graphics_queue.alloc_command_list());
        transition_list.open();
        for (upload_state& state : unique_resources)
        {
            transition_list.barrier(state.resource, state.resource_initial_state, ri_resource_state::common_state, ri_resource_state::initial);
        }
        transition_list.close();

        profile_gpu_marker(graphics_queue, profile_colors::gpu_transition, "transition resources to initial");
        graphics_queue.execute(transition_list);
    }

    m_stats_render_bytes_uploaded->submit(static_cast<double>(total_bytes));
}

}; // namespace ws
