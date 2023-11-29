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
//  Partitions an arbitrary numeric range into a heap and allows allocation/freeing of 
//  said range as though it was a memory heap, eg. with alloc/free.
// 
//  As this doesn't actually point to physical memory, but just a numeric range, you can use it
//  for tracking things such as gpu memory where you may have a size but not an actual pointer.
// 
//  TODO: This is pretty jankily implemented at the moment, its not efficient for large
//        numbers of allocations and shouldn't be used for time sensitive applications.
// ================================================================================================
class memory_heap
{
public:
    memory_heap(size_t size);
    ~memory_heap();

    // Allocates a block within the heap with the given size and alignment. Returns
    // false on failure. On success, the offset of the block in the heap will be returned
    // in offset.
    bool alloc(size_t size, size_t alignment, size_t& offset);

    // Frees a block in the heap previously allocated with alloc.
    void free(size_t offset);

    // Returns true if there are no allocations in the heap.
    bool empty();

    // Gets remaining size to be allocated.
    size_t get_remaining();

private:
    bool get_block_index(size_t offset, size_t& index);

    void coalesce(size_t index);

    struct block
    {
        size_t offset;
        size_t size;
        bool used = false;
    };

    std::vector<block> m_blocks;

    size_t m_remaining = 0;

};

}; // namespace workshop
