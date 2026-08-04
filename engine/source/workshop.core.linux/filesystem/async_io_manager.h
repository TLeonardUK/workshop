// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/filesystem/async_io_manager.h"
#include "workshop.core/math/rolling_average.h"
#include "workshop.core/math/rolling_rate.h"

#include <span>
#include <mutex>
#include <cstddef>
#include <unordered_map>

namespace ws {

class linux_async_io_manager;

class linux_async_io_request : public async_io_request
{
public:
    using ptr = std::shared_ptr<linux_async_io_request>;

    linux_async_io_request(linux_async_io_manager* manager, const char* path, size_t offset, size_t size, async_io_request_options options);
    virtual ~linux_async_io_request();

    virtual bool is_complete() override;
    virtual bool has_failed() override;
    virtual std::span<uint8_t> data() override;

private:

};

class linux_async_io_manager
    : public async_io_manager
{
public:
    linux_async_io_manager();
    ~linux_async_io_manager();

    // Gets the current IO bandwidth being used by all active requests in bytes per second.
    float get_current_bandwidth();

    // Starts a request to load the given block of data on the filesystem poinetd to by path
    // with the given offset and size.
    // 
    // No virtualization is performed on the path, this path is expected to be the 
    // raw on-disk path.
    async_io_request::ptr request(const char* path, size_t offset, size_t size, async_io_request_options options);

private:
    friend class linux_async_io_request;

private:

};

}; // namespace workshop
