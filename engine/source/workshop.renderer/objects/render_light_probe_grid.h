// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_object.h"

namespace ws {

// ================================================================================================
//  Represets a grid of diffuse light probes that the gpu can use for indirect lighting.
// ================================================================================================
class render_light_probe_grid : public render_object
{
public:
    render_light_probe_grid(render_object_id id, render_scene_manager* scene_manager, renderer& renderer);
    virtual ~render_light_probe_grid();

    // Gets/sets the density of the grid as a value that represents the distance between each probe.
    void set_density(float value);
    float get_density();

private:
    float m_density = 100.0f;

};

}; // namespace ws
