// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"

#include <unordered_map>

namespace ws {

// ================================================================================================
//  Provides the state and output of an attempt to compile a shader, including
//  any generated warning messages / errors or the like.
// ================================================================================================
class ri_shader_compiler_output
{
public:
    struct log
    {
        std::string message;
        std::string file;
        size_t line;
        size_t column;

        // Context contains multiple lines that point to the error in the code, eg for an unknown
        // identifier you might get something like this:
        // 
        //    identifer + 3;
        //    ^ 
        std::vector<std::string> context;
    };

    const std::vector<uint8_t>& get_bytecode() const
    {
        return m_bytecode;
    }

    const std::vector<log>& get_errors() const
    {
        return m_errors;
    }

    const std::vector<log>& get_warnings() const
    {
        return m_warnings;
    }

    const std::vector<log>& get_messages() const
    {
        return m_messages;
    }

    const std::vector<std::string>& get_dependencies() const
    {
        return m_dependencies;
    }

    void push_error(const log& message)
    {
        m_errors.push_back(message);
    }

    void push_warning(const log& message)
    {
        m_warnings.push_back(message);
    }

    void push_message(const log& message)
    {
        m_messages.push_back(message);
    }

    void push_dependency(const std::string& dependency)
    {
        m_dependencies.push_back(dependency);
    }

    void set_bytecode(std::vector<uint8_t>&& result)
    {
        m_bytecode = std::move(result);
    }

    bool success() const
    {
        return (m_errors.size() == 0 && m_bytecode.size() > 0);
    }

private:
    std::vector<log> m_errors;
    std::vector<log> m_warnings;
    std::vector<log> m_messages;
    std::vector<std::string> m_dependencies;

    std::vector<uint8_t> m_bytecode;

};

// ================================================================================================
//  Used to compiler shaders to bytecode that can be loaded directly by the renderer.
// 
//  Shaders should always be compiled offline, not at runtime. The libraries needed for 
//  compliation may not exist at runtime.
// ================================================================================================
class ri_shader_compiler
{
public:
    virtual ~ri_shader_compiler() {}

    // Attempts to compile the given shader.
    virtual ri_shader_compiler_output compile(
        ri_shader_stage stage, 
        const char* source,
        const char* file,
        const char* entrypoint,
        std::unordered_map<std::string, std::string>& defines,
        bool debug) = 0;

};

}; // namespace ws
