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
#include "workshop.windowing/window.h"

namespace ws {

dx12_ri_upload_manager::dx12_ri_upload_manager(dx12_render_interface& renderer)
    : m_renderer(renderer)
{
}

dx12_ri_upload_manager::~dx12_ri_upload_manager()
{
    if (m_heap_handle != nullptr)
    {
        D3D12_RANGE range;
        range.Begin = 0;
        range.End = k_heap_size;
        m_heap_handle->Unmap(0, &range);

        CheckedRelease(m_heap_handle);
    }

    m_heap_handle = nullptr;
    m_graphics_queue_fence = nullptr;
    m_copy_queue_fence = nullptr;
}

result<void> dx12_ri_upload_manager::create_resources()
{
    m_graphics_queue_fence = m_renderer.create_fence("Upload Manager - Graphics Fence");
    m_copy_queue_fence = m_renderer.create_fence("Upload Manager - Copy Fence");
    if (!m_graphics_queue_fence || !m_copy_queue_fence)
    {
        return false;
    }

    // Create the upload heap we will store all our cpu data in before its copied to the gpu.

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
        IID_PPV_ARGS(&m_heap_handle)       
    );
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateCommittedResource failed with error 0x%08x.", hr);
        return false;
    }

    D3D12_RANGE range;
    range.Begin = 0;
    range.End = k_heap_size;

    hr = m_heap_handle->Map(0, &range, (void**)&m_upload_heap_ptr);
    if (FAILED(hr))
    {
        db_error(render_interface, "Mapping upload heap failed with error 0x%08x.", hr);
        return false;
    }

    m_upload_heap = std::make_unique<memory_heap>(k_heap_size);

    return true;
}

void dx12_ri_upload_manager::upload(dx12_ri_texture& source, const std::span<uint8_t>& data)
{
    ri_command_queue& queue = m_renderer.get_copy_queue();

    size_t sub_resource_count = source.get_mip_levels() * source.get_depth();
    uint64_t total_memory = 0;
    std::vector<UINT> row_count;
    std::vector<UINT64> row_size;
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints;

    D3D12_RESOURCE_DESC desc = source.get_resource()->GetDesc();

    m_renderer.get_device()->GetCopyableFootprints(
        &desc,
        0,
        source.get_mip_levels() * source.get_depth(),
        0,
        footprints.data(),
        row_count.data(),
        row_size.data(),
        &total_memory
    );

    upload_state upload = allocate_upload(total_memory, 1);
    upload.texture = &source;

    // Copy source data into copy data. Source data is expected to be tightly packed
    // and stored linearly.
    uint8_t* source_data = data.data();

    for (size_t array_index = 0; array_index < source.get_depth(); array_index++)
    {
        for (size_t mip_index = 0; mip_index < source.get_mip_levels(); mip_index++)
        {
            const size_t sub_resource_index = mip_index + (array_index * source.get_mip_levels());

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint = footprints[sub_resource_index];
            size_t height = row_count[sub_resource_index];
            size_t pitch = math::round_up_multiple((size_t)footprint.Footprint.RowPitch, (size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
            size_t depth = footprint.Footprint.Depth;

            uint8_t* destination = (m_upload_heap_ptr + upload.heap_offset) + footprint.Offset;

            for (size_t slice_index = 0; slice_index < depth; slice_index++)
            {
                for (size_t height_index = 0; height_index < height; height_index++)
                {
                    size_t source_row_size = source.get_width() * ri_bytes_per_texel(source.get_format());

                    memcpy(destination, source_data, source_row_size);
                    if (source_row_size < pitch)
                    {
                        // Zero out the padding space.
                        memset(destination + source_row_size, 0, pitch - source_row_size);
                    }

                    source_data += source_row_size;
                }
            }
        }
    }

    // Generate copy command list.
    dx12_ri_command_list& list = static_cast<dx12_ri_command_list&>(queue.alloc_command_list());
    list.open();

    for (size_t i = 0; i < sub_resource_count; i++)
    {
        D3D12_TEXTURE_COPY_LOCATION dest = {};
        dest.pResource = source.get_resource();
        dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dest.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = m_heap_handle.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = footprints[i];
        src.PlacedFootprint.Offset += upload.heap_offset;

        list.get_dx_command_list()->CopyTextureRegion(
            &dest,
            0,
            0,
            0,
            &src,
            nullptr
        );
    }

    list.close();

    upload.list = &list;
    queue_upload(upload);
}

void dx12_ri_upload_manager::upload(dx12_ri_buffer& source, const std::span<uint8_t>& data)
{
    ri_command_queue& queue = m_renderer.get_copy_queue();

    upload_state upload = allocate_upload(data.size(), source.get_element_size());
    upload.buffer = &source;

    memcpy(m_upload_heap_ptr + upload.heap_offset, data.data(), data.size());

    dx12_ri_command_list& list = static_cast<dx12_ri_command_list&>(queue.alloc_command_list());
    list.open();
    list.get_dx_command_list()->CopyBufferRegion(
        source.get_resource(), 
        0, 
        m_heap_handle.Get(), 
        upload.heap_offset, 
        data.size()
    );
    list.close();

    upload.list = &list;
    queue_upload(upload);
}

dx12_ri_upload_manager::upload_state dx12_ri_upload_manager::allocate_upload(size_t size, size_t alignment)
{
    db_assert_message(size < k_heap_size, "Upload is larger than entire heap size. Increase the heap size if you need to upload this much data.");

    std::scoped_lock lock(m_pending_upload_mutex);

    upload_state state;

    if (!m_upload_heap->alloc(size, alignment, state.heap_offset))
    {
        db_fatal(renderer, "Upload heap has run out of space, consider increasing the size or reducing the amount uploaded.");
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
    size_t pipeline_depth = m_renderer.get_pipeline_depth();
    if (m_frame_index < pipeline_depth)
    {
        return;
    }

    size_t free_frame_index = m_frame_index - pipeline_depth; 

    for (auto iter = m_pending_free.begin(); iter != m_pending_free.end(); /* empty */)
    {
        if (iter->queued_frame_index <= free_frame_index)
        {
            m_upload_heap->free(iter->heap_offset);
            iter = m_pending_free.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

void dx12_ri_upload_manager::perform_uploads()
{
    ri_command_queue& copy_queue = m_renderer.get_copy_queue();
    ri_command_queue& graphics_queue = m_renderer.get_graphics_queue();

    size_t fence_value = m_frame_index + 1;

    std::vector<upload_state> uploads = m_pending_uploads;
    m_pending_uploads.clear();

    if (uploads.empty())
    {
        return;
    }

    // Transition all pending resources on graphics queue.
    dx12_ri_command_list& transition_list = static_cast<dx12_ri_command_list&>(graphics_queue.alloc_command_list());
    transition_list.open();
    for (upload_state& state : uploads)
    {
        if (state.texture)
        {
            transition_list.barrier(*state.texture, state.texture->get_initial_state(), ri_resource_state::copy_dest);
        }
        if (state.buffer)
        {
            transition_list.barrier(*state.buffer, state.buffer->get_initial_state(), ri_resource_state::copy_dest);
        }

        m_pending_free.push_back(state);
    }        
    transition_list.close();

    // Start transitioning resources on graphics queue.
    {
        profile_gpu_marker(graphics_queue, profile_colors::gpu_transition, "transition resources for upload");
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

        for (upload_state& state : uploads)
        {
            copy_queue.execute(*state.list);
        }
    }

    // Signal graphics queue that we have finished.
    m_copy_queue_fence->signal(copy_queue, fence_value);
}

}; // namespace ws
