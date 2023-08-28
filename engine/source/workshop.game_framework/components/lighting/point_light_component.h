// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"
#include "workshop.game_framework/components/lighting/light_component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/reflection/reflect.h"

#include "workshop.renderer/render_object.h"

namespace ws {

// ================================================================================================
//  Represents a point light in the world.
// ================================================================================================
class point_light_component : public light_component
{
public:

protected:

	friend class point_light_system;

    // Component is dirty and all settings need to be applied to render object.
    bool is_dirty = false;

public:

    BEGIN_REFLECT(point_light_component, "Point Light", light_component, reflect_class_flags::none)
    END_REFLECT()
};

}; // namespace ws
