// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.renderer/render_effect.h"

#include <unordered_map>

namespace ws {

class renderer;
class asset_manager;
class shader;

// ================================================================================================
//  Handles the creation, dispose and interpolation of all objects that exist 
//  in the render scene.
// ================================================================================================
class render_scene_manager
{
public:

    render_scene_manager(renderer& render);

    // Registers all the steps required to initialize the system.
    void register_init(init_list& list);

public:

    //void interpolate();
    //void process_queue();
    //void create_static_mesh();
    //void create_skeletal_mesh();
    //void create_

private:

    renderer& m_renderer;

};

}; // namespace ws
