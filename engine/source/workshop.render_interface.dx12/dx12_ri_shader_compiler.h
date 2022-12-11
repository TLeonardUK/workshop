// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_shader_compiler.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Implementation of a shader compiler for dx12.
// ================================================================================================
class dx12_ri_shader_compiler : public ri_shader_compiler
{
public:
    dx12_ri_shader_compiler(dx12_render_interface& renderer);
    virtual ~dx12_ri_shader_compiler();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    virtual ri_shader_compiler_output compile(
        ri_shader_stage stage,
        const char* source,
        const char* file,
        const char* entrypoint,
        std::unordered_map<std::string, std::string>& defines,
        bool debug) override;

protected:
    void parse_output(ri_shader_compiler_output& output, const std::string& text);

private:
    dx12_render_interface& m_renderer;

    Microsoft::WRL::ComPtr<IDxcLibrary> m_library;
    Microsoft::WRL::ComPtr<IDxcCompiler> m_compiler;

};

}; // namespace ws
