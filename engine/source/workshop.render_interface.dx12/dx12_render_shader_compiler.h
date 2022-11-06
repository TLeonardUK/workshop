// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/render_shader_compiler.h"
#include "workshop.core/utils/result.h"
#include "workshop.core.win32/utils/windows_headers.h"

#include <dxcapi.h>

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Implementation of a shader compiler for dx12.
// ================================================================================================
class dx12_render_shader_compiler : public render_shader_compiler
{
public:
    dx12_render_shader_compiler(dx12_render_interface& renderer);
    virtual ~dx12_render_shader_compiler();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    virtual render_shader_compiler_output compile(
        render_shader_stage stage,
        const char* source,
        const char* file,
        const char* entrypoint,
        std::unordered_map<std::string, std::string>& defines,
        bool debug) override;

protected:
    void parse_output(render_shader_compiler_output& output, const std::string& text);

private:
    dx12_render_interface& m_renderer;

    Microsoft::WRL::ComPtr<IDxcLibrary> m_library;
    Microsoft::WRL::ComPtr<IDxcCompiler> m_compiler;

};

}; // namespace ws
