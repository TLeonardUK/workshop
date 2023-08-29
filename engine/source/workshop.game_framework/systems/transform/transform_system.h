// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"

#include "workshop.engine/ecs/object.h"
#include "workshop.engine/ecs/system.h"

#include <unordered_set>
#include <vector>

namespace ws {

class transform_component;

// ================================================================================================
//  Updates object transforms inside a hierarchical graph.
// ================================================================================================
class transform_system : public system
{
public:
    transform_system(object_manager& manager);

    virtual void step(const frame_time& time) override;

    virtual void component_removed(object handle, component* comp) override;
    virtual void component_modified(object handle, component* comp) override;

public:

    // Public Commands

    // Sets the local transform of a given object.
    void set_local_transform(object handle, const vector3& location, const quat& rotation, const vector3& scale);
    
    // Sets the world transform of a given object.
    void set_world_transform(object handle, const vector3& location, const quat& rotation, const vector3& scale);

    // Sets the parent transform of a given object.
    void set_parent(object handle, object parent);

protected:

    void update_transform(transform_component* transform, transform_component* parent_transform);

private:

    // How many children a component needs before running in parallel.
    static constexpr inline size_t k_async_update_transform_threshold = 16;

    std::vector<std::unordered_set<transform_component*>> m_dirty_roots;
    std::vector<std::vector<transform_component*>> m_dirty_roots_list;

};

}; // namespace ws
