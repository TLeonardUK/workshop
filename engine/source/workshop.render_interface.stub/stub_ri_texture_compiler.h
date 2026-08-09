// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_texture_compiler.h"

namespace ws {

// ================================================================================================
//  Stub implementation of a texture compiler, performs no actual work.
// ================================================================================================
class stub_ri_texture_compiler : public ri_texture_compiler
{
public:
    virtual bool compile(
        ri_texture_dimension dimensions,
        size_t width,
        size_t height,
        size_t depth,
        std::vector<texture_face>& faces,
        std::vector<uint8_t>& output
    ) override;

};

}; // namespace ws
