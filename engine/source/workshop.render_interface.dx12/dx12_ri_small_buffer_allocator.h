// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface.dx12/dx12_headers.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.core/containers/memory_heap.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;
class dx12_ri_buffer;
class ri_buffer;

// ================================================================================================
//  This class handles allocating of small buffers that would normally end up creating a large 
//  amount of slack space if they were creating as their own resource due to page alignment.
// 
//  This works by allocating large buffers and sub-dividing them as needed. 
// ================================================================================================
class dx12_ri_small_buffer_allocator
{
public:
    struct handle
    {
        dx12_ri_buffer* buffer;
        size_t offset;
        size_t size;
    };

public:
    dx12_ri_small_buffer_allocator(dx12_render_interface& renderer);
    virtual ~dx12_ri_small_buffer_allocator();

    result<void> create_resources();

    bool alloc(size_t size, ri_buffer_usage usage, handle& out_handle);
    void free(handle in_handle);

    size_t get_max_size();

private:
    void add_new_buffer(ri_buffer_usage usage);

private:

    // Maximum size of allocation allowed inside the buffer allocator. 
    // Currently set to a single page size. This is fairly arbitrary, adjust as needed.
    static inline constexpr size_t k_max_allocation_size = std::numeric_limits<uint16_t>::max();

    // Size of each backing buffer that sub-buffers are packed into. Avoid making these two
    // small or you may end up with frequent churning as they are allocated/deallocated.
    static inline constexpr size_t k_buffer_size = 8 * 1024 * 1024;

    // Alignment of sub allocations.
    // 256 is a good number for this as its an an alignment that allows raytracing / constant buffer / etc all to 
    // consume these allocations.
    static inline constexpr size_t k_allocation_alignment = 256;

    struct buffer
    {
        std::unique_ptr<ri_buffer> buf;
        std::unique_ptr<memory_heap> heap;
        ri_buffer_usage usage;
    };

    std::mutex m_mutex;

    dx12_render_interface& m_renderer;
    std::vector<buffer> m_buffers;

};

}; // namespace ws
