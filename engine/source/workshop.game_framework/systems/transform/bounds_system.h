// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/math/obb.h"
#include "workshop.core/math/ray.h"

#include "workshop.core/containers/oct_tree.h"

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

    virtual void component_removed(object handle, component* comp) override;

    // Gets a obb that contains all the objects provided. If an object has no bounds assinged to it
    // then the default bounds will be used for it.
    obb get_combined_bounds(const std::vector<object>& objects, vector3& pivot_point, bool consume_pivot_point);

    // Returns all objects whos bounds intersect with the given ray.
    std::vector<object> intersects(const ray& target_ray);

private:
    
    // Bounds and extents of the octtree used to query objects.
    inline static const vector3 k_octtree_extents = vector3(1'000'000.0f, 1'000'000.0f, 1'000'000.0f);
    inline static const size_t k_octtree_max_depth = 10;

    // If object has no components we can calculate bounds from, we use this as the default.
    static inline constexpr float k_default_bounds = 100.0f;

    oct_tree<object> m_oct_tree;

};

}; // namespace ws
