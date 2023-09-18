// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/objects/render_light.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.core/utils/event.h"

namespace ws {

// ================================================================================================
//  Represets a spot light pointing in a specific direction with attenuation.
// ================================================================================================
class render_spot_light : public render_light
{
public:
    render_spot_light(render_object_id id, renderer& renderer);
    virtual ~render_spot_light();

    // Gets/sets the inner and outer radii of the spot lights umbra.
    void set_radius(float inner, float outer);
    void get_radius(float& inner, float& outer);

    // Overrides the default bounds to return the obb of the model bounds.
    virtual obb get_bounds() override;

private:

    // Updates the light state param block any any other render specific resources.
    virtual void update_render_data() override;

private:
    float m_inner_radius = 0.0f;
    float m_outer_radius = 0.0f;

};

}; // namespace ws
