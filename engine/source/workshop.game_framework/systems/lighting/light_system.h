// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"

#include "workshop.core/drawing/color.h"

namespace ws {

// ================================================================================================
//  Base class for all light-type systems, contains some generic functionality that is common
//  between all light types.
// ================================================================================================
class light_system : public system
{
public:

    light_system(object_manager& manager, const char* name);

public:

    // Public Commands

    void set_light_intensity(object id, float value);
    void set_light_range(object id, float value);
    void set_light_importance_distance(object id, float value);
    void set_light_color(object id, color color);
    void set_light_shadow_casting(object id, bool value);
    void set_light_shadow_map_size(object id, size_t value);
    void set_light_shadow_max_distance(object id, float value);

};

}; // namespace ws
