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
//  Represents a static mesh that always faces the camera.
// ================================================================================================
class billboard_component : public component
{
public:

	// The model this static mesh should render.
    // If no model is provided a plane will be used.
	asset_ptr<model> model;

	// What gpu flags are used to effect how this component is rendered.
    // TODO: Expose these in properties as list of tick boxes.
	render_draw_flags render_draw_flags = render_draw_flags::geometry;

    // What gpu flags are used to effect how this component is rendered.
    render_gpu_flags render_gpu_flags = render_gpu_flags::none;

    // Material overrides for model. 
    std::vector<asset_ptr<material>> materials;

    // Size of the billboard in world units.
    float size = 64.0f;

    // Local transform to align billboard with the camera.
    matrix4 transform = matrix4::identity;

private:

	friend class billboard_system;

	// ID of the render object in the renderer.
	render_object_id render_id = null_render_object;

	// Tracks the last transform we applied to the render view.
	size_t last_transform_generation = 0;

    // Component is dirty and all settings need to be applied to render object.
    bool is_dirty = false;

    // Flag for if the materials array needs to be regenerated. This occurs in situations such as
    // when the user modifies the model.
    bool materials_array_needs_update = false;

    // Used to track when model has been modified and refresh materials/etc.
    asset_ptr<ws::model> last_model;

public:

    BEGIN_REFLECT(billboard_component, "Billboard", component, reflect_class_flags::none)
        REFLECT_FIELD_REF(model,             "Model",        "Model asset this component displays, if none is provided a plane will be used.")
        REFLECT_FIELD_LIST_REF(materials,    "Materials",    "Materials to display on the meshes model.\nIf empty the defaults set in the model are used.")
        REFLECT_FIELD(size,                  "Size",         "Size that the billboard is displayed at.")
    END_REFLECT()

};

}; // namespace ws
