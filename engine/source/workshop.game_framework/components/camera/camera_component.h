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
//  Represents a camera rendering a view into the world.
// ================================================================================================
class camera_component : public component
{
public:

	// Field of view of the camera in degrees.
	float fov = 45.0f;

	// Aspect ration of the view.
	float aspect_ratio = 1.77f;

	// Minimum rendered depth of the view.
	float min_depth = 10.0f;

	// Maximum rendered depth of the view.
	float max_depth = 20000.0f;

private:

	friend class camera_system;

	// ID of the cameras view in the renderer.
	render_object_id view_id = null_render_object;

	// Tracks the last transform we applied to the render view.
	size_t last_transform_generation = 0;

};

}; // namespace ws
