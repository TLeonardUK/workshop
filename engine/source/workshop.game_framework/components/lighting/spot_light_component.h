// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"
#include "workshop.game_framework/components/lighting/light_component.h"

#include "workshop.core/math/math.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/reflection/reflect.h"

#include "workshop.renderer/render_object.h"

namespace ws {

// ================================================================================================
//  Represents a spot light in the world.
// ================================================================================================
class spot_light_component : public component
{
public:

	// The inner radius of the spotlight. The intensity is attenuated linearly between the radii.
	// The range is in radians between [0, pi]
	float inner_radius = 0.0f;
	
	// The outer radius of the spotlight. The intensity is attenuated linearly between the radii.
	// The range is in radians between [0, pi]
	float outer_radius = 0.2f;

protected:

	friend class spot_light_system;

	// Component is dirty and all settings need to be applied to render object.
	bool is_dirty = false;

public:

    BEGIN_REFLECT(spot_light_component, "Spot Light", component, reflect_class_flags::none)
        REFLECT_FIELD(inner_radius, "Inner Radius", "The inner radius of the spotlight. The intensity is attenuated linearly between the radii.\nThe range is in radians between [0, pi]")
        REFLECT_FIELD(outer_radius, "Outer Radius", "The outer radius of the spotlight. The intensity is attenuated linearly between the radii.\nThe range is in radians between [0, pi]")

        REFLECT_CONSTRAINT_RANGE(inner_radius, 0.0f, math::pi)

		REFLECT_DEPENDENCY(light_component)
	END_REFLECT()

};

}; // namespace ws
