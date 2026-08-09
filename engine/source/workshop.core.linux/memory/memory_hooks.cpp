// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/memory/memory.h"
#include "workshop.core/memory/memory_tracker.h"

#include <dlfcn.h>
#include <cstring>

// Unlike windows, which requires patching the import address tables of every loaded module,
// overriding the allocator on linux is done via the dynamic linkers standard symbol
// interposition rules - as long as this library is linked into the final binary, the malloc/
// free/calloc/realloc/posix_memalign definitions below automatically take priority over libcs.

// The one wrinkle is that dlsym() itself can call calloc internally the first time its used in
// a process (to allocate its own bookkeeping), which would otherwise recurse back into our
// calloc override before the real calloc has been resolved. The small static buffer below is
// the standard workaround for that bootstrap recursion.

namespace ws {

namespace {

using malloc_t = void* (*)(size_t);
using free_t = void (*)(void*);
using calloc_t = void* (*)(size_t, size_t);
using realloc_t = void* (*)(void*, size_t);
using posix_memalign_t = int (*)(void**, size_t, size_t);

static malloc_t s_real_malloc = nullptr;
static free_t s_real_free = nullptr;
static calloc_t s_real_calloc = nullptr;
static realloc_t s_real_realloc = nullptr;
static posix_memalign_t s_real_posix_memalign = nullptr;

static thread_local bool s_alloc_hook_reentrancy = false;

static bool s_resolving_real_functions = false;

constexpr size_t k_bootstrap_buffer_size = 16 * 1024;
static uint8_t s_bootstrap_buffer[k_bootstrap_buffer_size];
static size_t s_bootstrap_buffer_used = 0;

void* bootstrap_calloc(size_t num, size_t size)
{
    size_t alloc_size = num * size;
    if (s_bootstrap_buffer_used + alloc_size > k_bootstrap_buffer_size)
    {
        return nullptr;
    }

    void* ptr = &s_bootstrap_buffer[s_bootstrap_buffer_used];
    s_bootstrap_buffer_used += alloc_size;
    memset(ptr, 0, alloc_size);
    return ptr;
}

bool is_bootstrap_ptr(void* ptr)
{
    return ptr >= &s_bootstrap_buffer[0] && ptr < &s_bootstrap_buffer[k_bootstrap_buffer_size];
}

void resolve_real_functions()
{
    if (s_real_malloc != nullptr)
    {
        return;
    }

    s_resolving_real_functions = true;

    s_real_calloc = reinterpret_cast<calloc_t>(dlsym(RTLD_NEXT, "calloc"));
    s_real_malloc = reinterpret_cast<malloc_t>(dlsym(RTLD_NEXT, "malloc"));
    s_real_free = reinterpret_cast<free_t>(dlsym(RTLD_NEXT, "free"));
    s_real_realloc = reinterpret_cast<realloc_t>(dlsym(RTLD_NEXT, "realloc"));
    s_real_posix_memalign = reinterpret_cast<posix_memalign_t>(dlsym(RTLD_NEXT, "posix_memalign"));

    s_resolving_real_functions = false;
}

}; // namespace

void install_memory_hooks()
{
    db_log(core, "Installing memory hooks ...");
    resolve_real_functions();
}

}; // namespace ws

extern "C" void* malloc(size_t size)
{
    if (ws::s_resolving_real_functions)
    {
        return ws::bootstrap_calloc(1, size);
    }

    ws::resolve_real_functions();

    if (ws::s_alloc_hook_reentrancy)
    {
        return ws::s_real_malloc(size);
    }
    ws::s_alloc_hook_reentrancy = true;

    size_t alloc_size = size + ws::memory_tracker::k_raw_alloc_tag_size;
    void* ptr = ws::s_real_malloc(alloc_size);

    if (ws::memory_tracker::try_get())
    {
        ws::memory_tracker::get().record_raw_alloc(ptr, alloc_size);
    }

    ws::s_alloc_hook_reentrancy = false;

    return ptr;
}

extern "C" void* calloc(size_t num, size_t size)
{
    if (ws::s_resolving_real_functions)
    {
        return ws::bootstrap_calloc(num, size);
    }

    ws::resolve_real_functions();

    if (ws::s_alloc_hook_reentrancy)
    {
        return ws::s_real_calloc(num, size);
    }
    ws::s_alloc_hook_reentrancy = true;

    size_t alloc_size = (num * size) + ws::memory_tracker::k_raw_alloc_tag_size;
    void* ptr = ws::s_real_calloc(1, alloc_size);

    if (ws::memory_tracker::try_get())
    {
        ws::memory_tracker::get().record_raw_alloc(ptr, alloc_size);
    }

    ws::s_alloc_hook_reentrancy = false;

    return ptr;
}

extern "C" void* realloc(void* ptr, size_t new_size)
{
    ws::resolve_real_functions();

    if (ptr != nullptr && ws::is_bootstrap_ptr(ptr))
    {
        void* new_ptr = ws::bootstrap_calloc(1, new_size);
        if (new_ptr != nullptr)
        {
            memcpy(new_ptr, ptr, new_size);
        }
        return new_ptr;
    }

    if (ws::s_alloc_hook_reentrancy)
    {
        return ws::s_real_realloc(ptr, new_size);
    }
    ws::s_alloc_hook_reentrancy = true;

    if (ptr != nullptr)
    {
        if (ws::memory_tracker::try_get())
        {
            ws::memory_tracker::get().record_raw_free(ptr);
        }
    }

    size_t alloc_size = new_size + ws::memory_tracker::k_raw_alloc_tag_size;
    ptr = ws::s_real_realloc(ptr, alloc_size);
    ws::memory_tracker::get().record_raw_alloc(ptr, alloc_size);

    ws::s_alloc_hook_reentrancy = false;

    return ptr;
}

extern "C" void free(void* ptr)
{
    if (ptr == nullptr)
    {
        return;
    }

    if (ws::is_bootstrap_ptr(ptr))
    {
        return;
    }

    ws::resolve_real_functions();

    if (ws::s_alloc_hook_reentrancy)
    {
        ws::s_real_free(ptr);
        return;
    }
    ws::s_alloc_hook_reentrancy = true;

    if (ws::memory_tracker::try_get())
    {
        ws::memory_tracker::get().record_raw_free(ptr);
    }
    ws::s_real_free(ptr);

    ws::s_alloc_hook_reentrancy = false;
}

extern "C" int posix_memalign(void** memptr, size_t alignment, size_t size)
{
    ws::resolve_real_functions();

    return ws::s_real_posix_memalign(memptr, alignment, size);
}
