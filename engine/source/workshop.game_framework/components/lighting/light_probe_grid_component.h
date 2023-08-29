// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/reflection/reflect.h"

#include "workshop.renderer/render_object.h"

namespace ws {

// ================================================================================================
//  Represents a grid of diffuse light probes in the world.
// ================================================================================================
class light_probe_grid_component : public component
{
public:

	// Determines how much space there is between each light probe in the grid.
	float density = 100.0f;

private:

	friend class light_probe_grid_system;

	// ID of the render object in the renderer.
	render_object_id render_id = null_render_object;

	// Tracks the last transform we applied to the render object.
	size_t last_transform_generation = 0;

    // Component is dirty and all settings need to be applied to render object.
    bool is_dirty = false;

public:

    BEGIN_REFLECT(light_probe_grid_component, "Light Probe Grid", component, reflect_class_flags::none)
        REFLECT_FIELD(density,  "Density", "Determines how much space there is between each light probe in the grid.")

        REFLECT_CONSTRAINT_RANGE(density, 0.01f, 10000.0f)
    END_REFLECT()

};

}; // namespace ws
