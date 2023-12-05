// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_staging_buffer.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_upload_manager.h"
#include "workshop.render_interface.dx12/dx12_ri_texture.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"
#include "workshop.core/memory/memory_tracker.h"

namespace ws {

dx12_ri_staging_buffer::dx12_ri_staging_buffer(dx12_render_interface& renderer, dx12_ri_upload_manager& upload_manager, const ri_staging_buffer::create_params& params, std::span<uint8_t> data)
    : m_renderer(renderer)
    , m_upload_manager(upload_manager)
    , m_create_params(params)
    , m_data(data)
{
}

dx12_ri_staging_buffer::~dx12_ri_staging_buffer()
{
}

dx12_ri_upload_manager::upload_state dx12_ri_staging_buffer::take_upload_state()
{
    m_used = true;
    return m_upload;
}

result<void> dx12_ri_staging_buffer::create_resources()
{
    async_copy_manager& copy_manager = async_copy_manager::get();

    ri_command_queue& queue = m_renderer.get_copy_queue();

    dx12_ri_texture& dest = *static_cast<dx12_ri_texture*>(m_create_params.destination);

    // Calculate the offsets of each face in the stored data.
    size_t face_count = dest.get_depth();
    size_t mip_count = dest.get_mip_levels();
    size_t dropped_mip_count = dest.get_dropped_mips();
    size_t array_count = 1;
    if (dest.get_dimensions() == ri_texture_dimension::texture_cube)
    {
        face_count = 6;
        array_count = 6;
    }

    uint64_t total_memory = 0;

    UINT row_count;
    UINT64 row_size;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;

    UINT subresource_index = (UINT)((m_create_params.array_index * mip_count) + m_create_params.mip_index);

    D3D12_RESOURCE_DESC desc = dest.get_resource()->GetDesc();

    m_renderer.get_device()->GetCopyableFootprints(
        &desc,
        subresource_index,
        1,
        0u,
        &footprint,
        &row_count,
        &row_size,
        &total_memory
    );

    db_assert(total_memory >= m_data.size());

    m_upload = m_upload_manager.allocate_upload(total_memory, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    m_upload.resource = dest.get_resource();
    m_upload.resource_initial_state = dest.get_initial_state();
    m_upload.name = dest.get_debug_name();

    // Copy the source data into each subresource.
    size_t height = row_count;
    size_t pitch = math::round_up_multiple((size_t)footprint.Footprint.RowPitch, (size_t)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    size_t depth = footprint.Footprint.Depth;
    uint8_t* destination = (m_upload.heap->start_ptr + m_upload.heap_offset) + footprint.Offset;

    size_t source_data_offset = 0;

    // Copy source data into each row.
    size_t source_row_size = footprint.Footprint.Width * ri_bytes_per_texel(dest.get_format());

    // If pitch and row size are identical, coalesce into a single memcpy.
    if (pitch == source_row_size)
    {
        m_requests.push_back(copy_manager.request_memcpy(destination, m_data.data(), source_row_size * height));
    }
    else
    {
        size_t dest_offset = 0;
        for (size_t height_index = 0; height_index < height; height_index++)
        {
            db_assert(source_row_size == row_size);
            db_assert(dest_offset + source_row_size <= total_memory);
            db_assert(source_data_offset + source_row_size <= m_data.size());

            m_requests.push_back(copy_manager.request_memcpy(destination, m_data.data() + source_data_offset, source_row_size));
            
            source_data_offset += source_row_size;
            destination += pitch;
            dest_offset += pitch;
        }
    }

    return true;
}

bool dx12_ri_staging_buffer::is_staged()
{
    db_assert(!m_used);

    for (async_copy_request handle : m_requests)
    {
        if (!handle.is_complete())
        {
            return false;
        }
    }

    return true;
}

void dx12_ri_staging_buffer::wait()
{
    db_assert(!m_used);

    for (async_copy_request handle : m_requests)
    {
        if (!handle.is_complete())
        {
            handle.wait();
        }
    }
}

}; // namespace ws
