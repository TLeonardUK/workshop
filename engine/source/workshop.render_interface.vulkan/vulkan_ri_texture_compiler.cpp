// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_texture_compiler.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include <unordered_set>

namespace ws {

dx12_ri_texture_compiler::dx12_ri_texture_compiler(dx12_render_interface& renderer)
    : m_renderer(renderer)
{
}

dx12_ri_texture_compiler::~dx12_ri_texture_compiler()
{
}

result<void> dx12_ri_texture_compiler::create_resources()
{
    return true;
}

bool dx12_ri_texture_compiler::compile(
    ri_texture_dimension dimensions,
    size_t width,
    size_t height,
    size_t depth,
    std::vector<texture_face>& faces,
    std::vector<uint8_t>& output)
{    
    // TODO: Tightly packed data. We should do this fancier and match the format
    // as used at runtime, we need to build the correct resource_desc's/etc to do this.
    
    for (texture_face& face : faces)
    {
        for (auto& pix : face.mips)
        {
            output.insert(output.end(),
                pix->get_data().data(), 
                pix->get_data().data() + pix->get_data().size()
            );
        }
    }

    return true;
}

}; // namespace ws
