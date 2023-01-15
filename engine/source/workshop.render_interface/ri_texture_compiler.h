// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"

#include <unordered_map>

namespace ws {

// ================================================================================================
//  Used to compile a texture asset to a native format that can be directly loaded
//  by the render interface.
// 
//  Textures should be compiled offline where possible.
// ================================================================================================
class ri_texture_compiler
{
public:

    struct texture_face
    {
        std::vector<pixmap*> mips;
    };

    // Attempts to compile the given texture.
    virtual bool compile(
        ri_texture_dimension dimensions,
        size_t width,
        size_t height,
        size_t depth,
        std::vector<texture_face>& faces,
        std::vector<uint8_t>& output    
    ) = 0;

};

}; // namespace ws
