// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"

#include "workshop.renderer/render_object.h"

namespace ws {

// ================================================================================================
//  Represents a capture of the scene at a given point that is used for specular reflections.
// ================================================================================================
class reflection_probe_component : public component
{
public:

private:

	friend class reflection_probe_system;

	// ID of the render object in the renderer.
	render_object_id render_id = null_render_object;

	// Tracks the last transform we applied to the render object.
	size_t last_transform_generation = 0;

};

}; // namespace ws
