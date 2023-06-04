// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector2.h"

namespace ws {

// Represents an element within an instance buffer pointing to an individual instances data.
// Ensure this is kept in sync with the value in shader_loader.cpp
struct instance_offset_info
{
    uint32_t data_buffer_index;
    uint32_t data_buffer_offset;
};

}; // namespace ws
