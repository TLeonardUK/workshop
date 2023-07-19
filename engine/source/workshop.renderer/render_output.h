// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_texture.h"

#include <vector>

namespace ws {

class ri_texture;

// ================================================================================================
//  Represents the targets used to output the result of a render pass.
// ================================================================================================
class render_output
{
public:

    // Color targets, multiple can be used for MRT rendering.
    std::vector<ri_texture_view> color_targets;

    // Depth target used for depth reading/writing.
    ri_texture_view depth_target;

};

}; // namespace ws
