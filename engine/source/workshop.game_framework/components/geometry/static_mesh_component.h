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

#include "workshop.renderer/assets/model/model.h"

namespace ws {

// ================================================================================================
//  Represents a static model within the game world.
// ================================================================================================
class static_mesh_component : public component
{
public:

	// The model this static mesh should render.
	asset_ptr<model> model;

private:

	friend class static_mesh_system;

	// ID of the render object in the renderer.
	render_object_id render_id = null_render_object;

	// Tracks the last transform we applied to the render view.
	size_t last_transform_generation = 0;

    // Component is dirty and all settings need to be applied to render object.
    bool is_dirty = false;

public:

    BEGIN_REFLECT(static_mesh_component, "Static Mesh", component, reflect_class_flags::none)
        REFLECT_FIELD_REF(model,        "Model",     "Model asset this component displays.")
    END_REFLECT()

};

}; // namespace ws
