// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/utils/singleton.h"
#include "workshop.core/debug/debug.h"

#include <filesystem>

namespace ws {

/*
// Describes various operations that can be performed on a block of data loaded by the 
// the async_io_manager, such as decompression.
enum class async_io_request_options
{
    none = 0,

    // This is here for future expansion, its empty for now.
};

// ================================================================================================
//  This manager is responsible for loading blocks of data from the disk using 
//  async io to achieve near peak throughput.
// ================================================================================================
class async_io_manager
    : public singleton<async_io_manager>
{
public:
    async_io_manager();
    ~async_io_manager();

    // Gets the current IO bandwidth being used by all active requests in bytes per second.
    float get_current_bandwidth();

    // Starts a request to load the given block of data on the filesystem poinetd to by path
    // with the given offset and size.
    // 
    // No virtualization is performed on the path, this path is expected to be the 
    // raw on-disk path.
    std::unique_ptr<async_io_request> request(const char* path, size_t offset, size_t size, async_io_request_options options);
};
*/

}; // namespace workshop
