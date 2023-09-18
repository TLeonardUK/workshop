// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"

#include "workshop.core/drawing/color.h"

namespace ws {

// ================================================================================================
//  Updates basic light component work that is shared between all the extension types.
// ================================================================================================
class light_system : public system
{
public:

    light_system(object_manager& manager);

    virtual void step(const frame_time& time) override;

    virtual void component_removed(object handle, component* comp) override;
    virtual void component_modified(object handle, component* comp, component_modification_source source) override;

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
