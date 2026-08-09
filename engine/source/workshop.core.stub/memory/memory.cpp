// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/memory/memory.h"

#include <cstdlib>

namespace ws {

void* reserve_virtual_memory(size_t size)
{
    return std::malloc(size);
}

void free_virtual_memory(void* ptr)
{
    std::free(ptr);
}

void commit_virtual_memory(void* ptr, size_t size)
{
}

void decommit_virtual_memory(void* ptr, size_t size)
{
}

size_t get_page_size()
{
    return 4096;
}

}; // namespace ws
