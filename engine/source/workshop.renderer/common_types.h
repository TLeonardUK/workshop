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

// Maximum number of render views that can be active for tracking visibility/etc. Avoid increasing this unless
// neccessary as it adds overhead to every object. If you have more views than this you should reconsider why
// you are creating so many and if you can toggle them off.
static inline constexpr size_t k_max_render_views = 32;

}; // namespace ws
