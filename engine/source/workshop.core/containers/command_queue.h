// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/utils/traits.h"

#include <string>
#include <array>
#include <vector>
#include<span>

namespace ws {

// ================================================================================================
//  A base struct to derive comamnd_queue submitted commands from. This is purely for 
//  to show expected use, this class is expected to always be empty.
// ================================================================================================
struct command
{
};

// ================================================================================================
//  The command queue works as a FIFO buffer for command derived structs that contain POD data.
// 
//  The queues will continue to grow until reset is called, allowing
//  commands to allocate arbitrary blocks of data within the queue and know they will be valid 
//  for all commands until the reset is called. This is important to be aware of as reading
//  from the queue will not free any memory.
// 
//  Multiple threads can write to the queue at the same time.
//  Multiple threads can -NOT- read from the queue at a time.
//  Concurrent writes and reads are not valid. If this is required consider double buffering.
// 
//  The expected use case for this type is for situations such as many threads writing
//  commands to the queue (such as gameplay threads) and then a single thread processing 
//  the commands after they have all been written.
// ================================================================================================
class command_queue
{
private:

    // Header prefixed before each command or block of allocated data.
    struct command_header
    {
        size_t type_id;
        size_t size;
    };

    // Gets the command type id used for a block of arbitrary data. 
    // This makes the very-unlike-to-break assumption that no type has this id.
    static inline constexpr size_t k_data_command_id = std::numeric_limits<size_t>::max();

    // Gets a unique id for each command type.
    template <typename command_type>
    static constexpr size_t get_command_id()
    {
        return type_id<command_type>();
    }

public:
    command_queue(size_t capacity);
    ~command_queue();

    // Resets the queue back to its original state and erases all commands contained within it.
    void reset();

    // Returns true if the queue is empty.
    bool empty();

    // Gets the size in bytes that are actively in use in the queue.
    size_t size_bytes();

    // Returns true if the next command in the queue is of the given template type.
    template<typename command_type>
    bool peek_type();

    // Writes a command of the given type into the queue.
    template<typename command_type>
    void write(const command_type& command);

    // Reads the next command from the queue. Returns a nullptr if the next command in the queue
    // is not of the given type. If the command is not of the expected type it is not consumed, and
    // read should can be called again with a different type to query multiple types.
    template<typename command_type>
    command_type* read();

    // Same as read but will always read the next command as the given type even if the id missmatches.
    // Be very careful with this, you can easily cause faults if not handling types correctly.
    template<typename command_type>
    command_type* read_forced();

    // Allocates a block of data that can be referenced by commands. This can be used for commands that need to store large arrays of data. This data
    // will be deallocated automatically when free is called.
    std::span<uint8_t> allocate(size_t size);

    // Allocates a block of data that can contain the given primitive data and copies the value into it.
    const char* allocate_copy(const char* value);

private:

    // Allocates a raw block of data without a data command header.
    std::span<uint8_t> allocate_raw(size_t size);

    // Skips the next command in the queue.
    void skip_command();

    // Gets a unique id for each command type.
    command_header& peek_header();

    // Consumes the given number of bytes of data. Will assert
    // if more is consumed than is available.
    void consume(size_t bytes);

private:

    std::atomic_size_t m_write_offset = 0;
    std::atomic_size_t m_read_offset = 0;
    std::atomic_size_t m_non_data_command_read = 0;
    std::atomic_size_t m_non_data_command_written = 0;

    std::vector<uint8_t> m_buffer;

};

template<typename command_type>
bool command_queue::peek_type()
{
    static_assert(std::is_trivially_destructible<command_type>(), "Commands should be trivially destructable, use allocate() if you need dynamically sized memory.");
    db_assert_message(!empty(), "Attempted to peek an empty command queue.");

    size_t id = get_command_id<command_type>();
    return peek_header().type_id == id;
}

template<typename command_type>
void command_queue::write(const command_type& command)
{
    static_assert(std::is_trivially_destructible<command_type>(), "Commands should be trivially destructable, use allocate() if you need dynamically sized memory.");
    size_t id = get_command_id<command_type>();

    command_header header;
    header.type_id = id;
    header.size = sizeof(command_type);

    size_t data_required = sizeof(command_type) + sizeof(command_header);
    std::span<uint8_t> data = allocate_raw(data_required);
    memcpy(data.data(), &header, sizeof(command_header));
    memcpy(data.data() + sizeof(command_header), &command, sizeof(command_type));

    m_non_data_command_written.fetch_add(1);
}

template<typename command_type>
command_type* command_queue::read()
{
    static_assert(std::is_trivially_destructible<command_type>(), "Commands should be trivially destructable, use allocate() if you need dynamically sized memory.");
    db_assert_message(!empty(), "Attempted to read from empty command queue.");

    // Skip any data buffers preceding next command.
    while (!empty() && peek_header().type_id == k_data_command_id)
    {
        skip_command();
    }

    if (!peek_type<command_type>())
    {
        return nullptr;
    }

    command_header& header = peek_header();
    consume(sizeof(command_header) + header.size);
    m_non_data_command_read.fetch_add(1);

    return reinterpret_cast<command_type*>(reinterpret_cast<uint8_t*>(&header) + sizeof(command_header));
}

template<typename command_type>
command_type* command_queue::read_forced()
{
    static_assert(std::is_trivially_destructible<command_type>(), "Commands should be trivially destructable, use allocate() if you need dynamically sized memory.");
    db_assert_message(!empty(), "Attempted to read from empty command queue.");

    // Skip any data buffers preceding next command.
    while (!empty() && peek_header().type_id == k_data_command_id)
    {
        skip_command();
    }

    command_header& header = peek_header();
    consume(sizeof(command_header) + header.size);
    m_non_data_command_read.fetch_add(1);

    return reinterpret_cast<command_type*>(reinterpret_cast<uint8_t*>(&header) + sizeof(command_header));
}

}; // namespace workshop
