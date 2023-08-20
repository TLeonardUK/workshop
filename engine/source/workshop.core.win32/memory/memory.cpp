// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/memory/memory.h"

#include <Windows.h>

namespace ws {

void* reserve_virtual_memory(size_t size)
{
    void* ptr = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);
    if (!ptr)
    {
        db_fatal(core, "VirtualAlloc failed with 0x%08x", GetLastError());
    }
    return ptr;
}

void free_virtual_memory(void* ptr)
{
    if (!VirtualFree(ptr, 0, MEM_RELEASE))
    {
        db_fatal(core, "VirtualFree failed with 0x%08x", GetLastError());        
    }
}

void commit_virtual_memory(void* ptr, size_t size)
{
    void* ret = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    if (!ret)
    {
        db_fatal(core, "VirtualAlloc failed with 0x%08x", GetLastError());
    }
}

void decommit_virtual_memory(void* ptr, size_t size)
{
    if (!VirtualFree(ptr, size, MEM_DECOMMIT))
    {
        db_fatal(core, "VirtualAlloc failed with 0x%08x", GetLastError());
    }
}

size_t get_page_size()
{
    return 64 * 1024;
}

}; // namespace workshop
