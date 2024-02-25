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
//  Sphere collision shape.
// ================================================================================================
class physics_sphere_component : public component
{
public:

    float radius = 25.0f;

private:

	friend class physics_system;

public:

    BEGIN_REFLECT(physics_sphere_component, "Physics Sphere", component, reflect_class_flags::none)
        REFLECT_FIELD(radius, "Radius", "Radius of sphere.");

        REFLECT_DEPENDENCY(physics_component)
    END_REFLECT()

};

}; // namespace ws
