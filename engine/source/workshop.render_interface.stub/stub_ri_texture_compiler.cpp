// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.stub/stub_ri_texture_compiler.h"

namespace ws {

bool stub_ri_texture_compiler::compile(
    ri_texture_dimension dimensions,
    size_t width,
    size_t height,
    size_t depth,
    std::vector<texture_face>& faces,
    std::vector<uint8_t>& output)
{
    return false;
}

}; // namespace ws
