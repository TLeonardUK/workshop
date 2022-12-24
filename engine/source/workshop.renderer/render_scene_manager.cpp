// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_scene_manager.h"
#include "workshop.renderer/renderer.h"

namespace ws {
    
render_scene_manager::render_scene_manager(renderer& render)
    : m_renderer(render)
{
}

void render_scene_manager::register_init(init_list& list)
{
}

}; // namespace ws
