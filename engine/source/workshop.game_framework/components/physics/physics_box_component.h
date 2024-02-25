// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/reflection/reflect.h"

#include "workshop.game_framework/components/physics/physics_component.h"

namespace ws {

// ================================================================================================
//  Box collision shape.
// ================================================================================================
class physics_box_component : public component
{
public:

    vector3 extents = vector3(100.0f, 100.0f, 100.0f);

private:

	friend class physics_system;

public:

    BEGIN_REFLECT(physics_box_component, "Physics Box", component, reflect_class_flags::none)
        REFLECT_FIELD(extents, "Extents", "Extents of box along each axis.");

        REFLECT_DEPENDENCY(physics_component)
    END_REFLECT()

};

}; // namespace ws
