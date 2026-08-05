// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_shader_compiler.h"

namespace ws {

ri_shader_compiler_output vulkan_ri_shader_compiler::compile(
    ri_shader_stage stage,
    const char* source,
    const char* file,
    const char* entrypoint,
    std::unordered_map<std::string, std::string>& defines,
    bool debug)
{
    ri_shader_compiler_output output;

    ri_shader_compiler_output::log error;
    error.message = "Vulkan shader compilation is not yet implemented.";
    error.file = file ? file : "";
    error.line = 0;
    error.column = 0;
    output.push_error(error);

    return output;
}

}; // namespace ws
