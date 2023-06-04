// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"
#include "workshop.core/drawing/color.h"

#include <span>

namespace ws {

// Defines how a buffer is intended to be used.
enum class ri_buffer_usage
{
    generic,
    index_buffer,
    vertex_buffer
};

// ================================================================================================
//  Represents a block of gpu memory of arbitrary size.
// ================================================================================================
class ri_buffer
{
public:

    struct create_params
    {
        ri_buffer_usage usage = ri_buffer_usage::generic;

        size_t element_count = 0;
        size_t element_size = 1;

        // Linear data that we will upload into the buffer on construction.
        std::span<uint8_t> linear_data;
    };

public:
    virtual ~ri_buffer() {}

    virtual size_t get_element_count() = 0;
    virtual size_t get_element_size() = 0;

    virtual ri_resource_state get_initial_state() = 0;

    virtual void* map(size_t offset, size_t size) = 0;
    virtual void unmap(void* pointer) = 0;
    
};

}; // namespace ws
