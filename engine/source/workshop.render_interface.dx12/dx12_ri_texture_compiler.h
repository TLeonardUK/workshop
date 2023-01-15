// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_texture_compiler.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Implementation of a texture compiler for dx12.
// ================================================================================================
class dx12_ri_texture_compiler : public ri_texture_compiler
{
public:
    dx12_ri_texture_compiler(dx12_render_interface& renderer);
    virtual ~dx12_ri_texture_compiler();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    virtual bool compile(
        ri_texture_dimension dimensions,
        size_t width,
        size_t height,
        size_t depth,
        std::vector<texture_face>& faces,
        std::vector<uint8_t>& output
    ) override;

private:
    dx12_render_interface& m_renderer;

};

}; // namespace ws
