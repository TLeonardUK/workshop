// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"

namespace ws {

// ================================================================================================
//  Represents the position, scale and rotation of an object in 3d space.
// ================================================================================================
class transform_component : public component
{
public:

    //component_ref<transform_component> parent = null_object;

    // Transform relative to the parent.
    quat    local_rotation;
    vector3 local_location;
    vector3 local_scale;

};

}; // namespace ws
