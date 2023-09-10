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

	// What gpu flags are used to effect how this component is rendered.
	render_gpu_flags render_gpu_flags = render_gpu_flags::none;

    // Material overrides for model. 
    std::vector<asset_ptr<material>> materials;

private:

	friend class static_mesh_system;

	// ID of the render object in the renderer.
	render_object_id render_id = null_render_object;

	// Tracks the last transform we applied to the render view.
	size_t last_transform_generation = 0;

    // Component is dirty and all settings need to be applied to render object.
    bool is_dirty = false;

    // Keeps track of the last model this mesh had and does processing if its changed.
    asset_ptr<ws::model> old_model;

public:

    BEGIN_REFLECT(static_mesh_component, "Static Mesh", component, reflect_class_flags::none)
        REFLECT_FIELD_REF(model,             "Model",        "Model asset this component displays.")
        REFLECT_FIELD_LIST_REF(materials,    "Materials",    "Materials to display on the meshes model.\nIf empty the defaults set in the model are used.")
    END_REFLECT()

};

}; // namespace ws
