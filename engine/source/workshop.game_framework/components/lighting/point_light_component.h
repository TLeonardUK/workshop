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
class point_light_component : public component
{
protected:

	friend class point_light_system;

	// ID of the range render object in the renderer.
	render_object_id range_render_id = null_render_object;

	// Tracks the last transform we applied to the render object.
	size_t last_transform_generation = 0;

	// Object flags from last frame.
	object_flags last_flags = object_flags::unset;

public:

    BEGIN_REFLECT(point_light_component, "Point Light", component, reflect_class_flags::none)
        REFLECT_DEPENDENCY(light_component)
    END_REFLECT()

};

}; // namespace ws
