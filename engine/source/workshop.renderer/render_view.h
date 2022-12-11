// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/utils/frame_time.h"

#include "workshop.core/math/rect.h"
#include "workshop.core/math/frustum.h"

namespace ws {

// ================================================================================================
//  Represets a view into the scene to be renderer, including the associated projection
//  matrices, viewports and such.
// ================================================================================================
class render_view
{
public:

    std::string name = "Unnamed";

    recti viewport = recti::empty;

    float near_clip = 0.1f;
    float far_clip = 10000.0f;

    float field_of_view = 90.0f;
    float aspect_ratio = 1.33f;
    vector3 location = vector3::zero;
    quat rotation = quat::identity;

};

}; // namespace ws
