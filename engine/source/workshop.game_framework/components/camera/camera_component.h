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
#include "workshop.renderer/renderer.h"

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

    // Matrices calculated by the system.
    matrix4 projection_matrix;
    matrix4 view_matrix;

    // Render flags that dictate what gets drawn to this camera view.
    render_draw_flags draw_flags = render_draw_flags::geometry;

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

        REFLECT_CONSTRAINT_RANGE(fov,           1.0f,   170.0f)
        REFLECT_CONSTRAINT_RANGE(aspect_ratio,  0.1f,   10.0f)
        REFLECT_CONSTRAINT_RANGE(min_depth,     0.01f,  1000.0f)
        REFLECT_CONSTRAINT_RANGE(max_depth,     0.01f,  1000000.0f)
    END_REFLECT()

};

}; // namespace ws
