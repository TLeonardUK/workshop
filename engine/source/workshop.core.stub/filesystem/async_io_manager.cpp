// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/async_io_manager.h"
#include "workshop.core.stub/filesystem/async_io_manager.h"

#include <fstream>

namespace ws {

std::unique_ptr<async_io_manager> async_io_manager::create()
{
    return std::make_unique<stub_async_io_manager>();
}

stub_async_io_request::stub_async_io_request(const char* path, size_t offset, size_t size, async_io_request_options options)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open())
    {
        m_failed = true;
        return;
    }

    stream.seekg(static_cast<std::streamoff>(offset));

    m_data.resize(size);
    stream.read(reinterpret_cast<char*>(m_data.data()), static_cast<std::streamsize>(size));

    if (stream.gcount() != static_cast<std::streamsize>(size))
    {
        m_failed = true;
    }
}

bool stub_async_io_request::is_complete()
{
    return true;
}

bool stub_async_io_request::has_failed()
{
    return m_failed;
}

std::span<uint8_t> stub_async_io_request::data()
{
    return std::span<uint8_t>(m_data.data(), m_data.size());
}

float stub_async_io_manager::get_current_bandwidth()
{
    return 0.0f;
}

async_io_request::ptr stub_async_io_manager::request(const char* path, size_t offset, size_t size, async_io_request_options options)
{
    return std::make_shared<stub_async_io_request>(path, offset, size, options);
}

}; // namespace ws
