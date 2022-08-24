// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <string>

namespace ws {

// ================================================================================================
//  This class is the base class for protocol handlers that can be registered to the 
//  virtual file system.
// ================================================================================================
class stream
{
public:

    virtual ~stream() = default;

    // Flushes the stream and closes it, this is done implicitly on destruction.
    virtual void close() = 0;

    // Flushes any writes currently pending.
    virtual void flush() = 0;

    // Gets the position in the stream.
    virtual size_t position() = 0;

    // Gets the length of the entire stream.
    virtual size_t length() = 0;

    // Seeks to a specific location within the stream.
    virtual void seek(size_t position) = 0;

    // Writes the given bytes of data to the stream.
    // Returns the number of bytes written.
    virtual size_t write(const char* data, size_t size) = 0;

    // Reads the given bytes of data from the stream. 
    // Returns the number of bytes read.
    virtual size_t read(char* data, size_t size) = 0;

public:

    // Helper functions

    // Reads the entire streams contents in as a string.
    std::string read_all_string();
    
};

}; // namespace workshop
