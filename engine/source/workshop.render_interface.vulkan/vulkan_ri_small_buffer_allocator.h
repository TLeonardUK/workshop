// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.core/utils/result.h"

namespace ws {

class vulkan_render_interface;
class vulkan_ri_buffer;

// ================================================================================================
//  This class handles allocating of small buffers that would normally end up creating a large
//  amount of slack space if they were creating as their own resource due to page alignment.
//
//  This works by allocating large buffers and sub-dividing them as needed.
// ================================================================================================
class vulkan_ri_small_buffer_allocator
{
public:
    struct handle
    {
        vulkan_ri_buffer* buffer = nullptr;
        size_t offset = 0;
        size_t size = 0;
    };

public:
    vulkan_ri_small_buffer_allocator(vulkan_render_interface& renderer);
    ~vulkan_ri_small_buffer_allocator();

    result<void> create_resources();

    bool alloc(size_t size, ri_buffer_usage usage, handle& out_handle);
    void free(handle in_handle);

    size_t get_max_size();

};

}; // namespace ws
