// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/string.h"
#include "workshop.core/math/math.h"

namespace ws {

// Reserves a virtual address range.
void* reserve_virtual_memory(size_t size);

// Decommits and frees the given virtual address range.
void free_virtual_memory(void* ptr);

// Commits the give region into a virtual address range.
void commit_virtual_memory(void* ptr, size_t size);

// Decommits the give region into a virtual address range.
void decommit_virtual_memory(void* ptr, size_t size);

// Gets the memory page size used on the platform, as is used by the virtual memory functions.
size_t get_page_size();

}; // namespace ws::math