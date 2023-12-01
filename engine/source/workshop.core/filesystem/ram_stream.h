// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/filesystem/stream.h"
#include "workshop.core/utils/result.h"

#include <filesystem>
#include <stdio.h>

namespace ws {

// ================================================================================================
//  A stream that reads or writes to ram buffer.
// ================================================================================================
class ram_stream : public stream
{
public:

    ram_stream(const std::vector<uint8_t>& buffer);
    ram_stream(std::vector<uint8_t>& buffer, bool can_write);
    virtual ~ram_stream();

    virtual void close() override;
    virtual void flush() override;
    virtual bool can_write() override;
    virtual size_t position() override;
    virtual size_t length() override;
    virtual void seek(size_t position) override;
    virtual size_t write(const char* data, size_t size) override;
    virtual size_t read(char* data, size_t size) override;

    virtual std::string get_async_path() override;
    virtual size_t get_async_offset() override;

private:
    std::vector<uint8_t>& m_buffer;

    size_t m_position = 0;

    bool m_can_write = false;

};

}; // namespace workshop
