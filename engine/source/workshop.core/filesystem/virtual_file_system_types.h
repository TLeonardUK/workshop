// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

//#include <chrono>
#include <vector>
#include <memory>
#include <functional>

namespace ws {

// Represents what type of object a path points to.
// Can be used as a bitmask for certain functions.
enum class virtual_file_system_path_type
{
    non_existant = 0,
    file = 1,
    directory = 2
};


//using virtual_file_system_time_clock = std::chrono::utc_clock;
using virtual_file_system_time_point = uint64_t;//std::chrono::time_point<virtual_file_system_time_clock>;

// ================================================================================================
//  This is the base class for a path watcher which invokes a callback if the given 
//  path has been modified.
// ================================================================================================
class virtual_file_system_watcher
{
public:

    using callback_t = std::function<void(const char* path)>;

public:

    virtual ~virtual_file_system_watcher() {}

};

// ================================================================================================
//  This is just a container for a set of file system watchers from different handlers all
//  watching the same path.
// ================================================================================================
class virtual_file_system_watcher_compound : public virtual_file_system_watcher
{
public:

    std::vector<std::unique_ptr<virtual_file_system_watcher>> watchers;

};

}; // namespace workshop
