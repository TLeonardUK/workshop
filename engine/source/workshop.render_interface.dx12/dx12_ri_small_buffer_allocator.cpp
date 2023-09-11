// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_small_buffer_allocator.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"

namespace ws {

dx12_ri_small_buffer_allocator::dx12_ri_small_buffer_allocator(dx12_render_interface& renderer)
    : m_renderer(renderer)
{
}

dx12_ri_small_buffer_allocator::~dx12_ri_small_buffer_allocator()
{
}

result<void> dx12_ri_small_buffer_allocator::create_resources()
{
    // Nothing to do here right now ...
    return true;
}

bool dx12_ri_small_buffer_allocator::alloc(size_t size, ri_buffer_usage usage, handle& out_handle)
{
    std::scoped_lock lock(m_mutex);

    while (true)
    {
        // Search in existing buffers for one with space.
        for (buffer& buf : m_buffers)
        {
            if (buf.usage == usage && buf.heap->alloc(size, k_allocation_alignment, out_handle.offset))
            {
                out_handle.size = size;
                out_handle.buffer = static_cast<dx12_ri_buffer*>(buf.buf.get());
                return true;
            }
        }

        // Add a new buffer.
        add_new_buffer(usage);
    }

    return false;
}

void dx12_ri_small_buffer_allocator::free(handle in_handle)
{
    std::scoped_lock lock(m_mutex);

    for (buffer& buf : m_buffers)
    {
        if (in_handle.buffer == static_cast<dx12_ri_buffer*>(buf.buf.get()))
        {
            buf.heap->free(in_handle.offset);
            break;
        }
    }
}

size_t dx12_ri_small_buffer_allocator::get_max_size()
{
    return k_max_allocation_size;
}

void dx12_ri_small_buffer_allocator::add_new_buffer(ri_buffer_usage usage)
{
    ri_buffer::create_params params;
    params.element_count = k_buffer_size;
    params.element_size = 1;
    params.usage = usage;

    buffer& buf = m_buffers.emplace_back();
    buf.buf = m_renderer.create_buffer(params, "small buffers");
    buf.heap = std::make_unique<memory_heap>(k_buffer_size);
    buf.usage = usage;
}

}; // namespace ws
