// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"
#include "workshop.core/math/vector2.h"

namespace ws {

// ================================================================================================
//  Responsible for creating and updating render views from all active camera components.
// ================================================================================================
class camera_system : public system
{
public:

    camera_system(object_manager& manager);

    virtual void step(const frame_time& time) override;
 
    virtual void component_removed(object handle, component* comp) override;
    virtual void component_modified(object handle, component* comp, component_modification_source source) override;

public:

    // Public Commands

    // Sets the projection settings for a given camera.
    void set_projection(object handle, float fov, float aspect_ratio, float min_depth, float max_depth);

private:
    vector2 m_last_screen_size;

};

}; // namespace ws
