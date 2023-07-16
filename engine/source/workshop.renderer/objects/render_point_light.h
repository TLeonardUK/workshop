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
//  Represets a localized point light
// ================================================================================================
class render_point_light : public render_light
{
public:
    render_point_light(render_scene_manager* scene_manager, renderer& renderer);
    virtual ~render_point_light();

    // Overrides the default bounds to return the obb of the model bounds.
    virtual obb get_bounds() override;

private:

    // Updates the light state param block any any other render specific resources.
    virtual void update_render_data() override;

private:

};

}; // namespace ws
