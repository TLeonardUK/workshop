// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/math/obb.h"

#include "workshop.engine/ecs/object.h"
#include "workshop.engine/ecs/system.h"

#include <unordered_set>
#include <vector>

namespace ws {

class transform_component;

// ================================================================================================
//  Updates object bounds
// ================================================================================================
class bounds_system : public system
{
public:
    bounds_system(object_manager& manager);

    virtual void step(const frame_time& time) override;

    // Gets a obb that contains all the objects provided. If an object has no bounds assinged to it
    // then the default bounds will be used for it.
    obb get_combined_bounds(const std::vector<object>& objects, float default_bounds, vector3& pivot_point, bool consume_pivot_point);

};

}; // namespace ws
