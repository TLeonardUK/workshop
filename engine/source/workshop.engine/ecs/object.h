// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <cstdint>

namespace ws {

// ================================================================================================
//  Acts as an opaque handle representing a specific object within the world.
// ================================================================================================
using object = uint64_t;
static inline constexpr object null_object = 0;

}; // namespace ws
