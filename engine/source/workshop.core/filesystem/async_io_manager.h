// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/utils/singleton.h"
#include "workshop.core/debug/debug.h"

#include <span>
#include <memory>

namespace ws {

// Describes various operations that can be performed on a block of data loaded by the 
// the async_io_manager, such as decompression.
enum class async_io_request_options
{
    none = 0,

    // This is here for future expansion, its empty for now.
};

// ================================================================================================
//  Represents an outstanding IO request.
// ================================================================================================
class async_io_request
{
public:

    using ptr = std::shared_ptr<async_io_request>;

    // Returns true once this request has completed.
    virtual bool is_complete() = 0;

    // Returns true if the request failed for any reason.
    virtual bool has_failed() = 0;

    // Gets the that was loaded from disk.
    virtual std::span<uint8_t> data() = 0;
  
};

// ================================================================================================
//  This manager is responsible for loading blocks of data from the disk using 
//  async io to achieve near peak throughput.
// ================================================================================================
class async_io_manager
    : public singleton<async_io_manager>
{
public:

    // Creates thje platform specific implementation of the io manager.
    static std::unique_ptr<async_io_manager> create();

    // Gets the current IO bandwidth being used by all active requests in bytes per second.
    virtual float get_current_bandwidth() = 0;

    // Starts a request to load the given block of data on the filesystem poinetd to by path
    // with the given offset and size.
    // 
    // No virtualization is performed on the path, this path is expected to be the 
    // raw on-disk path.
    virtual async_io_request::ptr request(const char* path, size_t offset, size_t size, async_io_request_options options) = 0;

};

}; // namespace workshop
