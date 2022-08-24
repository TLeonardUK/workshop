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
//  A stream that reads or writes to a file on disk
// ================================================================================================
class disk_stream : public stream
{
public:
    
    virtual ~disk_stream();

    result<void> open(const std::filesystem::path& path, bool for_writing);

    virtual void close() override;
    virtual void flush() override;
    virtual size_t position() override;
    virtual size_t length() override;
    virtual void seek(size_t position) override;
    virtual size_t write(const char* data, size_t size) override;
    virtual size_t read(char* data, size_t size) override;

private:
    FILE* m_file;

    uint64_t m_length;

};

}; // namespace workshop
