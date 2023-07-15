// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_object.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.core/utils/event.h"

namespace ws {

// ================================================================================================
//  Represets a global light casting in a specific direction, eg the sun.
// ================================================================================================
class render_directional_light : public render_object
{
public:
    render_directional_light(render_scene_manager* scene_manager, renderer& renderer);
    virtual ~render_directional_light();

    // Gets/sets if the light will cast a shdow.
    void set_shadow_casting(bool value);
    bool get_shodow_casting();

    // Gets/sets the size of the shadow map texture.
    void set_shadow_map_size(size_t value);
    size_t get_shodow_map_size();

    // Gets/sets the maximum distance at which shadows are rendered, which defines what the cascades are fitted to.
    void set_shadow_max_distance(float value);
    float get_shodow_max_distance();

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

    // Gets the light_state block that describes the light in a shader.
    ri_param_block* get_light_state_param_block();

private:

    virtual void set_local_transform(const vector3& location, const quat& rotation, const vector3& scale) override;
    
    // Updates the light state param block any any other render specific resources.
    void update_render_data();

private:
    renderer& m_renderer;

    bool m_shadow_casting = false;
    size_t m_shadow_map_size = 512;
    size_t m_shadow_map_cascades = 3;
    float m_shadow_map_exponent = 0.5f;
    float m_shadow_max_distance = 0.0f;
    float m_shadow_cascade_blend = 0.0f;

    std::unique_ptr<ri_param_block> m_light_state_param_block;

};

}; // namespace ws
