// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/memory/memory.h"

#include <sys/mman.h>
#include <unistd.h>

#include <mutex>
#include <unordered_map>

namespace ws {

namespace {

std::mutex g_reservation_mutex;
std::unordered_map<void*, size_t> g_reservation_sizes;

}; // namespace

void* reserve_virtual_memory(size_t size)
{
    void* ptr = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (ptr == MAP_FAILED)
    {
        db_fatal(core, "mmap failed with errno %i", errno);
        return nullptr;
    }

    std::lock_guard lock(g_reservation_mutex);
    g_reservation_sizes[ptr] = size;

    return ptr;
}

void free_virtual_memory(void* ptr)
{
    size_t size;
    {
        std::lock_guard lock(g_reservation_mutex);
        auto iter = g_reservation_sizes.find(ptr);
        if (iter == g_reservation_sizes.end())
        {
            db_fatal(core, "free_virtual_memory called with an untracked pointer");
            return;
        }
        size = iter->second;
        g_reservation_sizes.erase(iter);
    }

    if (munmap(ptr, size) != 0)
    {
        db_fatal(core, "munmap failed with errno %i", errno);
    }
}

void commit_virtual_memory(void* ptr, size_t size)
{
    if (mprotect(ptr, size, PROT_READ | PROT_WRITE) != 0)
    {
        db_fatal(core, "mprotect failed with errno %i", errno);
    }
}

void decommit_virtual_memory(void* ptr, size_t size)
{
    if (mprotect(ptr, size, PROT_NONE) != 0)
    {
        db_fatal(core, "mprotect failed with errno %i", errno);
    }
    madvise(ptr, size, MADV_DONTNEED);
}

size_t get_page_size()
{
    static size_t page_size = static_cast<size_t>(sysconf(_SC_PAGESIZE));
    return page_size;
}

}; // namespace workshop
