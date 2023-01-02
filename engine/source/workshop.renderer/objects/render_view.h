// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/rect.h"
#include "workshop.core/math/frustum.h"

#include "workshop.renderer/render_object.h"

namespace ws {

// ================================================================================================
//  Represets a view into the scene to be renderer, including the associated projection
//  matrices, viewports and such.
// ================================================================================================
class render_view : public render_object
{
public:

    recti viewport = recti::empty;

    float near_clip = 0.1f;
    float far_clip = 10000.0f;

    float field_of_view = 90.0f;
    float aspect_ratio = 1.33f;

};

}; // namespace ws
