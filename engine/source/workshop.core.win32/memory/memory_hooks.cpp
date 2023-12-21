// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/memory/memory.h"
#include "workshop.core/memory/memory_tracker.h"

#include <Windows.h>

// The functions in this are a little questionable and are heavily dependent on windows not
// changing how their crt is setup. But is mostly fairly solid, and several games have been
// shipped using similar behaviour.

// This code essentially works by finding the ordinals we care about, then modifying the 
// address of their thunk in the import address table of the module they exist in.

// Be careful modifying this if your not aware how it works, its fragile.

namespace ws {

class memory_hook_base
{
public:
    virtual const char* get_name() = 0;
    virtual void load_address(HMODULE mod, const char* mod_name) = 0;
    virtual void* get_hook_function() = 0;

};

template <typename signature_t>
class memory_hook : public memory_hook_base
{
public:
    memory_hook(const char* name, signature_t replacement)
        : m_name(name)
        , m_replacement_function(replacement)
    {
    }

    virtual const char* get_name() override
    {
        return m_name;
    }

    virtual void load_address(HMODULE mod, const char* mod_name) override
    {
        if (m_original_function)
        {
            return;
        }
        m_original_function = (signature_t)GetProcAddress(mod, m_name);
#if 0
        if (m_original_function)
        {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), "Found memory function: module=%s function=%s\n", mod_name, m_name);
            OutputDebugStringA(buffer);
        }
#endif
    }

    virtual void* get_hook_function() override
    {
        return m_replacement_function;
    }

    signature_t get_original()
    {
        return m_original_function;
    }
    
private:
    const char* m_name;
    signature_t m_original_function;
    signature_t m_replacement_function;
};

// All the hooks we use.
extern "C" void* __cdecl override_malloc(size_t size);
memory_hook s_malloc_hook("malloc", &override_malloc);

extern "C" void* __cdecl override_realloc(void* ptr, size_t new_size);
memory_hook s_realloc_hook("realloc", &override_realloc);

extern "C" void* __cdecl override_calloc(size_t num, size_t size);
memory_hook s_calloc_hook("calloc", &override_calloc);

extern "C" void __cdecl override_free(void* ptr);
memory_hook s_free_hook("free", &override_free);

extern "C" void* __cdecl override_recalloc(void* ptr, size_t num, size_t size);
memory_hook s_recalloc_hook("_recalloc", &override_recalloc);

extern "C" void __cdecl override_aligned_free(void* ptr);
memory_hook s_aligned_free_hook("_aligned_free", &override_aligned_free);

extern "C" void* __cdecl override_aligned_malloc(size_t size, size_t alignment);
memory_hook s_aligned_malloc_hook("_aligned_malloc", &override_aligned_malloc);

extern "C" void* __cdecl override_aligned_realloc(void* ptr, size_t size, size_t alignment);
memory_hook s_aligned_realloc_hook("_aligned_realloc", &override_aligned_realloc);

extern "C" void* __cdecl override_aligned_recalloc(void* ptr, size_t num, size_t size, size_t alignment);
memory_hook s_aligned_recalloc_hook("_aligned_recalloc", &override_aligned_recalloc);

extern "C" void* __cdecl override_aligned_offset_malloc(size_t size, size_t alignment, size_t offset);
memory_hook s_aligned_offset_malloc_hook("_aligned_offset_malloc", &override_aligned_offset_malloc);

extern "C" void* __cdecl override_aligned_offset_realloc(void* ptr, size_t size, size_t alignment, size_t offset);
memory_hook s_aligned_offset_realloc_hook("_aligned_offset_realloc", &override_aligned_offset_realloc);

extern "C" void* __cdecl override_aligned_offset_recalloc(void* ptr, size_t num, size_t size, size_t alignment, size_t offset);
memory_hook s_aligned_offset_recalloc_hook("_aligned_offset_recalloc", &override_aligned_offset_recalloc);

// Array containing every hook we need to install.
static inline memory_hook_base* s_memory_hooks[] = {
    &s_malloc_hook,
    &s_realloc_hook,
    &s_calloc_hook,
    &s_free_hook,
    &s_recalloc_hook,
    &s_aligned_free_hook,
    &s_aligned_malloc_hook,
    &s_aligned_realloc_hook,
    &s_aligned_recalloc_hook,
    &s_aligned_offset_malloc_hook,
    &s_aligned_offset_realloc_hook,
    &s_aligned_offset_recalloc_hook
};

void hook_module(HMODULE mod, std::vector<HMODULE>& imported_modules)
{
    // Check we've not already hooked this module.
    if (std::find(imported_modules.begin(), imported_modules.end(), mod) != imported_modules.end())
    {
        return;
    }
    imported_modules.push_back(mod);

    // Iterate through the thunk table of the module, and do some questionable things to it to replace
    // the thunk destination with our override.
    void* image_base = (void*)mod;
    PIMAGE_DOS_HEADER dos_headers = (PIMAGE_DOS_HEADER)image_base;
    PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)((DWORD_PTR)image_base + dos_headers->e_lfanew);

    IMAGE_DATA_DIRECTORY import_directory = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    PIMAGE_IMPORT_DESCRIPTOR import_desc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD_PTR)import_directory.VirtualAddress + (DWORD_PTR)image_base);

    while (import_desc->Name != 0)
    {
        const char* lib_name = (LPCSTR)((DWORD_PTR)import_desc->Name + (DWORD_PTR)image_base);
        HMODULE lib = LoadLibraryA(lib_name);

#if 0
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "library: %s\n", lib_name);
        OutputDebugStringA(buffer);
#endif

        if (lib)
        {
            // Hook anything this library imports.
            hook_module(lib, imported_modules);

            // Grab the original functions for each hook from this module.
            for (int j = 0; j < ARRAYSIZE(s_memory_hooks); j++)
            {
                s_memory_hooks[j]->load_address(lib, lib_name);
            }

            PIMAGE_THUNK_DATA original_first_thunk = (PIMAGE_THUNK_DATA)((DWORD_PTR)image_base + import_desc->OriginalFirstThunk);
            PIMAGE_THUNK_DATA first_thunk = (PIMAGE_THUNK_DATA)((DWORD_PTR)image_base + import_desc->FirstThunk);

            while (original_first_thunk->u1.AddressOfData != 0)
            {
                if ((original_first_thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) != IMAGE_ORDINAL_FLAG)
                {
                    PIMAGE_IMPORT_BY_NAME function_name = (PIMAGE_IMPORT_BY_NAME)((DWORD_PTR)image_base + original_first_thunk->u1.AddressOfData);

#if 0
                    snprintf(buffer, sizeof(buffer), "\tfunction: %s\n", function_name->Name);
                    OutputDebugStringA(buffer);
#endif

                    for (int j = 0; j < ARRAYSIZE(s_memory_hooks); j++)
                    {
                        memory_hook_base* hook = s_memory_hooks[j];

                        if (strcmp(hook->get_name(), function_name->Name) == 0)
                        {
#if 0
                            snprintf(buffer, sizeof(buffer), "Found and installed IAT hook for %s\n", hook->get_name());
                            OutputDebugStringA(buffer);
#endif

                            DWORD old_protection = 0;
                            VirtualProtect((LPVOID)(&first_thunk->u1.Function), 8, PAGE_READWRITE, &old_protection);

                            first_thunk->u1.Function = (DWORD_PTR)hook->get_hook_function();
                            break;
                        }
                    }
                }

                original_first_thunk++;
                first_thunk++;
            }
        }

        import_desc++;
    }
}

void install_memory_hooks()
{
    std::vector<HMODULE> imported_modules; 
    imported_modules.resize(1024); // Make sure we have enough space before we start trying to hook memory functions.
    
    (void)_aligned_malloc(1024, 32);

    db_log(core, "Installing memory hooks ...");
    hook_module(GetModuleHandle(NULL), imported_modules);
}

// Hook implementations
thread_local bool s_alloc_hook_renetrancy = false;

extern "C" void* __cdecl override_malloc(size_t size)
{
    if (s_alloc_hook_renetrancy)
    {
        return s_malloc_hook.get_original()(size);
    }
    s_alloc_hook_renetrancy = true;

    size_t alloc_size = size + memory_tracker::k_raw_alloc_tag_size;
    void* ptr = s_malloc_hook.get_original()(alloc_size);
    memory_tracker::get().record_raw_alloc(ptr, alloc_size);

    s_alloc_hook_renetrancy = false;

    return ptr;
}

extern "C" void* __cdecl override_realloc(void* ptr, size_t new_size)
{
    if (s_alloc_hook_renetrancy)
    {
        return s_realloc_hook.get_original()(ptr, new_size);
    }
    s_alloc_hook_renetrancy = true;

    if (ptr)
    {
        memory_tracker::get().record_raw_free(ptr);
    }

    size_t alloc_size = new_size + memory_tracker::k_raw_alloc_tag_size;
    ptr = s_realloc_hook.get_original()(ptr, alloc_size);
    memory_tracker::get().record_raw_alloc(ptr, alloc_size);

    s_alloc_hook_renetrancy = false;

    return ptr;
}

extern "C" void* __cdecl override_calloc(size_t num, size_t size)
{
    if (s_alloc_hook_renetrancy)
    {
        return s_calloc_hook.get_original()(num, size);
    }
    s_alloc_hook_renetrancy = true;

    // So based on the documentation, this isn't correct as the size may not always be num*size
    // due to alignment requirements, but in practice it seems fine. 
    // Worth a double check.

    size_t alloc_size = (num * size) + memory_tracker::k_raw_alloc_tag_size;
    void* ptr = s_calloc_hook.get_original()(1, alloc_size);
    memory_tracker::get().record_raw_alloc(ptr, alloc_size);

    s_alloc_hook_renetrancy = false;

    return ptr;
}

extern "C" void __cdecl override_free(void* ptr)
{
    if (s_alloc_hook_renetrancy)
    {
        return s_free_hook.get_original()(ptr);
    }
    s_alloc_hook_renetrancy = true;

    if (ptr)
    {
        memory_tracker::get().record_raw_free(ptr);
    }
    s_free_hook.get_original()(ptr);

    s_alloc_hook_renetrancy = false;
}

extern "C" void* __cdecl override_recalloc(void* ptr, size_t num, size_t size)
{
    if (s_alloc_hook_renetrancy)
    {
        return s_recalloc_hook.get_original()(ptr, num, size);
    }
    s_alloc_hook_renetrancy = true;

    if (ptr)
    {
        memory_tracker::get().record_raw_free(ptr);
    }

    size_t alloc_size = (num * size) + memory_tracker::k_raw_alloc_tag_size;
    ptr = s_recalloc_hook.get_original()(ptr, 1, alloc_size);
    memory_tracker::get().record_raw_alloc(ptr, alloc_size);

    s_alloc_hook_renetrancy = false;

    return ptr;
}




extern "C" void __cdecl override_aligned_free(void* ptr)
{
    return s_aligned_free_hook.get_original()(ptr);
}

extern "C" void* __cdecl override_aligned_malloc(size_t size, size_t alignment)
{
    return s_aligned_malloc_hook.get_original()(size, alignment);
}

extern "C" void* __cdecl override_aligned_realloc(void* ptr, size_t size, size_t alignment)
{
    return s_aligned_realloc_hook.get_original()(ptr, size, alignment);
}

extern "C" void* __cdecl override_aligned_recalloc(void* ptr, size_t num, size_t size, size_t alignment)
{
    return s_aligned_recalloc_hook.get_original()(ptr, num, size, alignment);
}

extern "C" void* __cdecl override_aligned_offset_malloc(size_t size, size_t alignment, size_t offset)
{
    return s_aligned_offset_malloc_hook.get_original()(size, alignment, offset);
}

extern "C" void* __cdecl override_aligned_offset_realloc(void* ptr, size_t size, size_t alignment, size_t offset)
{
    return s_aligned_offset_realloc_hook.get_original()(ptr, size, alignment, offset);
}

extern "C" void* __cdecl override_aligned_offset_recalloc(void* ptr, size_t num, size_t size, size_t alignment, size_t offset)
{
    return s_aligned_offset_recalloc_hook.get_original()(ptr, num, size, alignment, offset);
}

}; // namespace workshop
