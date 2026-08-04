// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/memory/memory.h"

#include <Windows.h>

namespace ws {

void* reserve_virtual_memory(size_t size)
{
    // linux-todo
}

void free_virtual_memory(void* ptr)
{
    // linux-todo
}

void commit_virtual_memory(void* ptr, size_t size)
{
    // linux-todo
}

void decommit_virtual_memory(void* ptr, size_t size)
{
    // linux-todo
}

size_t get_page_size()
{
    // linux-todo
}

}; // namespace workshop
