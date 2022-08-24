// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

// Represents what type of object a path points to.
// Can be used as a bitmask for certain functions.
enum class virtual_file_system_path_type
{
    non_existant = 0,
    file = 1,
    directory = 2
};

}; // namespace workshop
