// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"

#include <string>

namespace ws {

// ================================================================================================
//  Base class for all types of objects that exist within the render scene - meshes, views, etc.
// ================================================================================================
class render_object
{
public:
    virtual ~render_object() {};

public:

    // Name of the effect, use for debugging.
    std::string name;

    // Transformation.
    quat    local_rotation;
    vector3 local_location;
    vector3 local_scale;

};

}; // namespace ws
