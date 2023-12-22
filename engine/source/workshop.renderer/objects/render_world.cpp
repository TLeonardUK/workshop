// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_world.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_resource_cache.h"
#include "workshop.renderer/systems/render_system_debug.h"

namespace ws {

render_world::render_world(render_object_id id, renderer& renderer)
    : render_object(id, &renderer, render_visibility_flags::none)
{
}

render_world::~render_world()
{
}

}; // namespace ws
