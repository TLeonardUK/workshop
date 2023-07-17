// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_pass.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.core/hashing/hash.h"

namespace ws {

void* render_pass::get_cache_key(render_view& view)
{
    size_t hash = 0;
    hash_combine(hash, system);
    hash_combine(hash, name);
    hash_combine(hash, view.get_id());
    return reinterpret_cast<void*>(hash);
}

}; // namespace ws
