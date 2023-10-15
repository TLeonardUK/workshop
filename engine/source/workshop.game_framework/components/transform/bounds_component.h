// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/matrix4.h"
#include "workshop.core/math/obb.h"
#include "workshop.core/containers/oct_tree.h"
#include "workshop.core/reflection/reflect.h"

namespace ws {

// ================================================================================================
//  Represents the bounds of an object in 3d space.
// ================================================================================================
class bounds_component : public component
{
public:

    // Bounds of the object in local space.
    obb local_bounds;

    // Bounds of the object in world space.
    obb world_bounds;

protected:

    friend class bounds_system;

    // Tracks the last transform we applied to the render object.
    size_t last_transform_generation = 0;

    // Last local transform of the model (for billboards etc).
    matrix4 last_model_transform = matrix4::identity;

    // Tracks the last model used for calculating bounds.
    size_t last_model_version = 0;
    size_t last_model_hash = 0;

    // True if bounds have been calculated and ready for use. 
    bool is_valid = false;

    // True if component has valid components to calculate bounds from. If false, a default
    // bounds will be calculated.
    bool has_bounds_source = false;

    // Represents an entry into the bounds octree.
    oct_tree<object>::token octree_token;

public:

    BEGIN_REFLECT(bounds_component, "Bounds", component, reflect_class_flags::none)
    END_REFLECT()

};

}; // namespace ws
