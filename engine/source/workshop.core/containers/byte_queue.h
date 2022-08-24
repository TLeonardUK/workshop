// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/containers/string.h"

#include <string>
#include <array>
#include <vector>

namespace ws {

// ================================================================================================
//  Simple queue that allows you to enqueue and deqeueue arbitrary byte buffers.
//  This is implemented as "chunked" circle buffer.
//  This class is thread safe, but not atomic.
// ================================================================================================
class byte_queue
{
public:
 
    ~byte_queue();

    // Gets the number of bytes waiting to be read in the queue.
    size_t pending_bytes();

    // Dequeues a given a given number of bytes into the provided buffer.
    // Returns false if there is not enough bytes in the queue to statisfy the request. 
    bool dequeue(uint8_t* buffer, size_t length);

    // Peeks the given number of bytes into the provided buffer but does not dequeue them.
    // Returns false if there is not enough bytes in the queue to statisfy the request. 
    bool peek(uint8_t* buffer, size_t length);

    // Enqueues a given buffer of bytes.
    void enqueue(const uint8_t* buffer, size_t length);

    // Clears all contents of the queue.
    void empty();

private:

    inline const static size_t k_block_length = 1024 * 16;

    struct block
    {
        std::array<uint8_t, k_block_length> buffer;
        size_t read_index;
        size_t write_index;

        block* next;
    };

    std::mutex m_mutex;

    block* m_queued_list_head = nullptr;
    block* m_queued_list_tail = nullptr;
    block* m_free_list = nullptr;

    std::atomic_size_t m_pending_bytes;

private:

    // Does some basic validity checking on the queue, namely for debugging.
    void check_validity();

    // Asserts that a given block is in the full linked list started.
    void assert_in_head_check(block* in);

};


}; // namespace workshop
