// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"
#include "workshop.renderer/assets/model/model.h"

namespace ws {

// ================================================================================================
//  Responsible for creating and updating render objects for static meshes.
// ================================================================================================
class static_mesh_system : public system
{
public:

    static_mesh_system(object_manager& manager);

    virtual void step(const frame_time& time) override;
 
    virtual void component_removed(object handle, component* comp) override;
    virtual void component_modified(object handle, component* comp) override;

public:

    // Public Commands

    void set_model(object handle, asset_ptr<model> model);

    void set_render_gpu_flags(object handle, render_gpu_flags flags);

};

}; // namespace ws
