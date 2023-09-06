// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/debug/debug.h"

#include <array>

namespace ws {

std::string stream::read_all_string()
{
    std::string result;
    result.resize(length());
    read(result.data(), result.size());
    return std::move(result);
}

bool stream::copy_to(stream& destination)
{
    const size_t block_size = 1024 * 64;
    std::array<char, block_size> data;

    while (!at_end())
    {
        size_t bytes_left = remaining();
        size_t to_read = std::min(bytes_left, block_size);

        size_t bytes_read = read(data.data(), to_read);
        if (bytes_read != to_read)
        {
            db_assert_message(false, "Read unexpected number of bytes from stream, expected %zi got %zi.", bytes_read, to_read);
            return false;
        }

        size_t bytes_written = destination.write(data.data(), bytes_read);
        if (bytes_written != bytes_read)
        {
            db_assert_message(false, "Wrote unexpected number of bytes from stream, expected %zi got %zi.", bytes_read, bytes_written);
            return false;
        }
    }

    return true;
}

size_t stream::remaining()
{
    return length() - position();
}

bool stream::at_end()
{
    return remaining() == 0;
}

}; // namespace workshop
