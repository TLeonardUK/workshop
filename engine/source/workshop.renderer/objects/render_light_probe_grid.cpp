// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_light_probe_grid.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.render_interface/ri_interface.h"

namespace ws {

render_light_probe_grid::render_light_probe_grid(render_object_id id, render_scene_manager* scene_manager, renderer& renderer)
    : render_object(id, scene_manager)
{
}

render_light_probe_grid::~render_light_probe_grid()
{
}

void render_light_probe_grid::set_density(float value)
{
    m_density = value;
}

float render_light_probe_grid::get_density()
{
    return m_density;
}

}; // namespace ws
