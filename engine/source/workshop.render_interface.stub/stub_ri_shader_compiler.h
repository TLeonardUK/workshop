// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_shader_compiler.h"

namespace ws {

// ================================================================================================
//  Stub implementation of a shader compiler, performs no actual work.
// ================================================================================================
class stub_ri_shader_compiler : public ri_shader_compiler
{
public:
    virtual ri_shader_compiler_output compile(
        ri_shader_stage stage,
        const char* source,
        const char* file,
        const char* entrypoint,
        std::unordered_map<std::string, std::string>& defines,
        bool debug) override;

};

}; // namespace ws
