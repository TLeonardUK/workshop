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
//  Represets a global light casting in a specific direction, eg the sun.
// ================================================================================================
class render_directional_light : public render_light
{
public:
    render_directional_light(render_object_id id, renderer& renderer);
    virtual ~render_directional_light();

    // Gets/sets the number of shadow map cascades 
    void set_shadow_cascades(size_t value);
    size_t get_shodow_cascades();

    // Gets/sets the exponent from which the shadow map cascade split will be derived.
    // The lower the exponent the closer to linear the split becomes.
    void set_shadow_cascade_exponent(float value);
    float get_shodow_cascade_exponent();

    // Gets/sets the fraction of a cascade that is blended into the next cascade.
    void set_shadow_cascade_blend(float value);
    float get_shodow_cascade_blend();

    // Overrides the default bounds to return the obb of the model bounds.
    virtual obb get_bounds() override;

    virtual void debug_draw(render_system_debug& debug) override;

private:

    // Updates the light state param block any any other render specific resources.
    virtual void update_render_data() override;

    // Builds shadow map resources.
    void build_shadow_maps();

private:
    size_t m_shadow_map_size = 512;
    size_t m_shadow_map_cascades = 3;
    float m_shadow_map_exponent = 0.75f;
    float m_shadow_map_blend = 0.1f;

};

}; // namespace ws
