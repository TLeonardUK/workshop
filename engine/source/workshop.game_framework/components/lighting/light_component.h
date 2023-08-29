// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/drawing/color.h"

#include "workshop.renderer/render_object.h"

namespace ws {

// ================================================================================================
//  Base class for all light type components. Holds various generic properties that apply
// 	to all light types.
// ================================================================================================
class light_component : public component
{
public:

	// Arbitrary scale to the lights radiance.
	float intensity = 1.0f;

	// Maximum distance away from light that it attenuates to nothing.
	float range = 10000.0f;

	// How far away the view has to be from the light before its faded out.
	float importance_range = 5000.0f;

	// Color of the light.
	color color = color::white;
	
	// If true this light will cast shadows.
	bool shadow_casting = false;

	// The size of the texture map that is used to render the lights view for shadow casting.
	// Larger sizes will give less aliased results but will cost considerably more memory.
	size_t shadow_map_size = 512;

	// Maximum distance from the light before the shadwo factor is faded out.
	float shadow_map_distance = 3000.0f;

protected:

	friend class light_system;

	// ID of the render object in the renderer.
	render_object_id render_id = null_render_object;

	// Tracks the last transform we applied to the render object.
	size_t last_transform_generation = 0;

public:

    BEGIN_REFLECT(light_component, "Light Component", component, reflect_class_flags::abstract)
        REFLECT_FIELD(intensity,            "Intensity",                "Arbitrary scale to the lights radiance.")
        REFLECT_FIELD(range,                "Range",                    "Maximum distance away from light that it attenuates to nothing.")
        REFLECT_FIELD(importance_range,     "Importance Range",         "How far away the view has to be from the light before its faded out.")
        REFLECT_FIELD(color,                "Color",                    "Color of the light.")
        REFLECT_FIELD(shadow_casting,       "Shadow Casting",           "If true this light will cast shadows.")
        REFLECT_FIELD(shadow_map_size,      "Shadow Map Size",          "The size of the texture map that is used to render the lights view for shadow casting.")
        REFLECT_FIELD(shadow_map_distance,  "Shadow Map Distance",      "Maximum distance from the light before the shadwo factor is faded out.")


        REFLECT_CONSTRAINT_RANGE(intensity,             0.01f, 1000000.0f)
        REFLECT_CONSTRAINT_RANGE(range,                 0.01f, 1000000.0f)
        REFLECT_CONSTRAINT_RANGE(importance_range,      0.01f, 1000000.0f)
        REFLECT_CONSTRAINT_RANGE(shadow_map_size,       64,    16384)
        REFLECT_CONSTRAINT_RANGE(shadow_map_distance,   0.01f, 1000000.0f)
    END_REFLECT()

};

}; // namespace ws
