// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"
#include "workshop.renderer/assets/model/model.h"

namespace ws {

// ================================================================================================
//  Responsible for updating physics bodies for all collision components.
// ================================================================================================
class physics_system : public system
{
public:

    physics_system(object_manager& manager);

    virtual void step(const frame_time& time) override;
 
    virtual void component_removed(object handle, component* comp) override;
    virtual void component_modified(object handle, component* comp, component_modification_source source) override;

public:

    // Public Commands

protected:

    void draw_debug();

};

}; // namespace ws
