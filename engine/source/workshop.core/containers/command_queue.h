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
//  The command queue works as a FIFO buffer for commands.
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
public:
    command_queue(size_t capacity);
    ~command_queue();

    // Resets the queue back to its original state and erases all commands contained within it.
    void reset();

    // Returns true if the queue is empty.
    bool empty();

    // Gets the size in bytes that are actively in use in the queue.
    size_t size_bytes();

    // Writes a command of the given type into the queue.
    // Name is used to describe the command. Its lifetime needs to remain until the command is executed
    // so use a literal, or allocate_copy a string for it.
    template<typename lambda_type>
    void queue_command(const char* name, lambda_type&& lambda);

    // Reads the next command from the queue and executes it.
    void execute_next();

    // Allocates a block of data that can be referenced by commands. This can be used for commands that need to store large arrays of data. This data
    // will be deallocated automatically when free is called.
    std::span<uint8_t> allocate(size_t size);

    // Allocates a block of data that can contain the given primitive data and copies the value into it.
    const char* allocate_copy(const char* value);

private:

    // Allocates a raw block of data without a data command header.
    std::span<uint8_t> allocate_raw(size_t size);

private:

    using execute_function_t = void (*)(void* data);

    struct command_header
    {
        execute_function_t execute;
        const char* name;
        void* lambda_pointer;
        command_header* next;
    };

    std::atomic_size_t m_write_offset = 0;    
    std::vector<uint8_t> m_buffer;

    std::atomic<command_header*> m_command_head;
    std::atomic<command_header*> m_command_tail;

};

template<typename lambda_type>
void command_queue::queue_command(const char* name, lambda_type&& lambda)
{
    // Allocate and move lambda into our buffer.
    std::span<uint8_t> lambda_data = allocate_raw(sizeof(lambda_type));
    lambda_type* placed_lambda = new(lambda_data.data()) lambda_type(std::move(lambda));

    // Allocate a new command header.
    std::span<uint8_t> command_header_data = allocate_raw(sizeof(command_header));
    command_header* header = new(command_header_data.data()) command_header;
    header->name = name;
    header->next = nullptr;
    header->lambda_pointer = placed_lambda;
    header->execute = [](void* data) {
        lambda_type* strong_type = reinterpret_cast<lambda_type*>(data);
        (*strong_type)();
        strong_type->~lambda_type();
    };

    // Commit command header.
    while (true)
    {
        command_header* last = m_command_tail.load();
        if (m_command_tail.compare_exchange_strong(last, header))
        {
            if (last)
            {
                last->next = header;
            }
            else
            {
                m_command_head = header;
            }
            break;
        }
    }
}

}; // namespace workshop
