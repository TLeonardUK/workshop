// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"

namespace ws {

// ================================================================================================
//  Responsible for creating and updating render objects for probe grids.
// ================================================================================================
class light_probe_grid_system : public system
{
public:

    light_probe_grid_system(object_manager& manager);

    virtual void step(const frame_time& time) override;
 
    virtual void component_removed(object handle, component* comp) override;
    virtual void component_modified(object handle, component* comp) override;

public:

    // Public Commands

    void set_grid_density(object handle, float value);


};

}; // namespace ws
