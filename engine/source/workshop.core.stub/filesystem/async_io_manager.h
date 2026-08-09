// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/filesystem/async_io_manager.h"

#include <vector>
#include <string>

namespace ws {

// ================================================================================================
//  Stub implementation of an async io request. The stub platform has no asynchronous file io
//  apis available, so requests are actually completed synchronously on creation.
// ================================================================================================
class stub_async_io_request : public async_io_request
{
public:
    stub_async_io_request(const char* path, size_t offset, size_t size, async_io_request_options options);

    virtual bool is_complete() override;
    virtual bool has_failed() override;
    virtual std::span<uint8_t> data() override;

private:
    bool m_failed = false;
    std::vector<uint8_t> m_data;

};

// ================================================================================================
//  Stub implementation of the async io manager, performs synchronous reads.
// ================================================================================================
class stub_async_io_manager
    : public async_io_manager
{
public:
    virtual float get_current_bandwidth() override;

    virtual async_io_request::ptr request(const char* path, size_t offset, size_t size, async_io_request_options options) override;

};

}; // namespace ws
