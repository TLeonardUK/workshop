// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_shader_compiler.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.vulkan/vulkan_headers.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atlbase.h>
#endif

#include <dxcapi.h>

namespace ws {

class vulkan_render_interface;

// ================================================================================================
//  Implementation of a shader compiler for vulkan.
// ================================================================================================
class vulkan_ri_shader_compiler : public ri_shader_compiler
{
public:
    vulkan_ri_shader_compiler(vulkan_render_interface& renderer);
    virtual ~vulkan_ri_shader_compiler();

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
    vulkan_render_interface& m_renderer;

    CComPtr<IDxcLibrary> m_library;
    CComPtr<IDxcCompiler> m_compiler;

};

}; // namespace ws
