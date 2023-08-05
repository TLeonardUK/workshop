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
//  Base class for all light types.
// ================================================================================================
class render_light : public render_object
{
public:
    render_light(render_object_id id, renderer& renderer, render_visibility_flags visibility_flags);
    virtual ~render_light();

    // Gets/sets the color of the light.
    void set_color(color value);
    color get_color();

    // Gets/sets the intensity of the light.
    void set_intensity(float value);
    float get_intensity();

    // Gets/sets the maximum range of the light.
    void set_range(float value);
    float get_range();

    // Gets/sets the distance from the camera before the light's contribution is faded out.
    void set_importance_distance(float value);
    float get_importance_distance();

    // Gets/sets if the light will cast a shdow.
    void set_shadow_casting(bool value);
    bool get_shodow_casting();

    // Gets/sets the size of the shadow map texture.
    void set_shadow_map_size(size_t value);
    size_t get_shadow_map_size();

    // Gets/sets the maximum distance at which shadows are rendered, which defines what the cascades are fitted to.
    void set_shadow_max_distance(float value);
    float get_shadow_max_distance();

    // Gets the light_state block that describes the light in a shader.
    ri_param_block* get_light_state_param_block();

protected:

    virtual void set_local_transform(const vector3& location, const quat& rotation, const vector3& scale) override;
    
    // Updates the light state param block any any other render specific resources.
    virtual void update_render_data();

protected:
    color m_color = color::white;
    float m_intensity = 0.0f;
    float m_range = 10000.0f;
    float m_importance_distance = 5000.0f;

    bool m_shadow_casting = false;
    size_t m_shadow_map_size = 512;
    float m_shadow_max_distance = 3000.0f;

    std::unique_ptr<ri_param_block> m_light_state_param_block;

};

}; // namespace ws
