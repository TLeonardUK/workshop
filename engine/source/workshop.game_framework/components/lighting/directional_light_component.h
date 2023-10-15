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
//  Represents a directional light in the world.
// ================================================================================================
class directional_light_component : public component
{
public:

	// Number of cacades the directional light has. Later cascades cover larger and larger areas. 
	// Having multiple allows for good shadow detail and large distances than having a single one
	// that covers the same area.
	size_t shadow_cascades = 3;

	// Determines how the cascades are split across the viewing frustum.
	// The lower the exponent the closer to linear the split becomes.
	float shadow_cascade_exponent = 0.75f;

	// Sets the fraction of a cascade that is used to blend between it and the next cascade. 
	// Provides a gradual transition between the cascades.
	float shadow_cascade_blend = 0.1f;

protected:

	friend class directional_light_system;
	
	// ID of the render object for displaying the range.
	render_object_id range_render_id = null_render_object;

	// Tracks the last transform we applied to the render object.
	size_t last_transform_generation = 0;

    // Component is dirty and all settings need to be applied to render object.
    bool is_dirty = false;

	// Object flags from last frame.
	object_flags last_flags = object_flags::unset;

public:

    BEGIN_REFLECT(directional_light_component, "Directional Light", component, reflect_class_flags::none)
        REFLECT_FIELD(shadow_cascades,              "Shadow Cascade Count",             "Number of cacades the directional light has. Higher numbers of casts provide more detailed coverage across longer distances, but require more memory and gpu time.");
        REFLECT_FIELD(shadow_cascade_exponent,      "Shadow Cascade Exponent",          "Determines how the cascades are split across the viewing frustum.\nThe lower the exponent the closer to linear the split becomes.");
        REFLECT_FIELD(shadow_cascade_blend,         "Shadow Cascade Blend",             "The fraction of a cascade that is used to blend between it and the next cascade.\nProvides a gradual transition between the cascades.");

        REFLECT_CONSTRAINT_RANGE(shadow_cascades,           1,      6)
        REFLECT_CONSTRAINT_RANGE(shadow_cascade_exponent,   0.0f,   1.0f)
        REFLECT_CONSTRAINT_RANGE(shadow_cascade_blend,      0.0f,   1.0f)

		REFLECT_DEPENDENCY(light_component)
    END_REFLECT()

};

}; // namespace ws
