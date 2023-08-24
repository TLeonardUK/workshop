// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"

namespace ws {

// ================================================================================================
//  Responsible for creating and updating render objects for reflection probes
// ================================================================================================
class reflection_probe_system : public system
{
public:

    reflection_probe_system(object_manager& manager);

    virtual void step(const frame_time& time) override;
 
    virtual void component_removed(object handle, component* comp) override;

public:

    // Public Commands


};

}; // namespace ws
