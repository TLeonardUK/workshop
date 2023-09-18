// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"

namespace ws {

// ================================================================================================
//  Responsible for creating and updating render lights for point lights..
// ================================================================================================
class point_light_system : public system
{
public:

    point_light_system(object_manager& manager);

    virtual void step(const frame_time& time) override;
 
    virtual void component_removed(object handle, component* comp) override;
    virtual void component_modified(object handle, component* comp, component_modification_source source) override;

public:

    // Public Commands


};

}; // namespace ws
