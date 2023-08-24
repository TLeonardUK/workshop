// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"
#include "workshop.game_framework/systems/lighting/light_system.h"

namespace ws {

// ================================================================================================
//  Responsible for creating and updating render lights for directional lights..
// ================================================================================================
class directional_light_system : public light_system
{
public:

    directional_light_system(object_manager& manager);

    virtual void step(const frame_time& time) override;
 
    virtual void component_removed(object handle, component* comp) override;

public:

    // Public Commands

    void set_light_shadow_cascades(object handle, size_t shadow_cascades);
    void set_light_shadow_cascade_exponent(object handle, float value);
    void set_light_shadow_cascade_blend(object handle, float value);

};

}; // namespace ws
