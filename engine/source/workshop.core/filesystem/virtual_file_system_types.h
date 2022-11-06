// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <chrono>

namespace ws {

// Represents what type of object a path points to.
// Can be used as a bitmask for certain functions.
enum class virtual_file_system_path_type
{
    non_existant = 0,
    file = 1,
    directory = 2
};


using virtual_file_system_time_clock = std::chrono::utc_clock;
using virtual_file_system_time_point = std::chrono::time_point<virtual_file_system_time_clock>;

}; // namespace workshop
