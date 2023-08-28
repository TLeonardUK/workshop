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

    // Component is dirty and all settings need to be applied to render object.
    bool is_dirty = false;

public:

    BEGIN_REFLECT(camera_component, "Camera", component, reflect_class_flags::none)
        REFLECT_FIELD(fov,              "Field Of View",    "Field of view of the camera in degrees.")
        REFLECT_FIELD(aspect_ratio,     "Aspect Ratio",     "Aspect ratio of the view, should normally be the proportion between width and height.")
        REFLECT_FIELD(min_depth,        "Min Depth",        "Minimum z value that can be seen by the view, defines the near clipping plane.")
        REFLECT_FIELD(max_depth,        "Max Depth",        "Maximum z value that can be seen by the view, defines the far clipping plane.")
    END_REFLECT()

};

}; // namespace ws
