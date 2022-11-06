// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/assets/asset.h"
#include "workshop.core/containers/string.h"

#include "workshop.render_interface/render_types.h"

#include <array>
#include <unordered_map>

namespace ws {

class asset;

// ================================================================================================
//  Shader files contain a description of the param blocks, render state, techniques
//  and other associated rendering data required to use a shader as part of
//  a render pass.
// ================================================================================================
class shader : public asset
{
public:

    struct param_block
    {
        enum class scopes
        {
            global,
            instance,

            COUNT
        };

        inline static const char* scopes_strings[static_cast<int>(scopes::COUNT)] = {
            "global",
            "instance"
        };

        struct field
        {
            std::string name;
            render_data_type data_type;
        };

        std::string name;
        scopes scope;
        std::vector<field> fields;
    };

    struct render_state
    {
        std::string name;
        render_pipeline_state state;
    };

    struct variation
    {
        std::string name;
        std::vector<std::string> values;
    };

    struct vertex_layout
    {
        struct field
        {
            std::string name;
            render_data_type data_type;
        };

        std::string name;
        std::vector<field> fields;
    };

    struct effect
    {
        struct technique
        {
            std::string name;
            std::vector<variation> variations;
        };

        std::string name;
        std::vector<technique> techniques;
    };

    struct technique
    {
        struct stage
        {
            std::string file;
            std::string entry_point;
            std::vector<uint8_t> bytecode;
        };

        std::string name;
        std::array<stage, static_cast<int>(render_shader_stage::COUNT)> stages;
        size_t render_state_index;
        size_t vertex_layout_index;
        std::vector<size_t> param_block_indices;
        std::unordered_map<std::string, std::string> defines;
    };

public:

    std::vector<param_block> param_blocks;
    std::vector<render_state> render_states;
    std::vector<variation> variations;
    std::vector<vertex_layout> vertex_layouts;
    std::vector<effect> effects;
    std::vector<technique> techniques;

};

DEFINE_ENUM_TO_STRING(shader::param_block::scopes, shader::param_block::scopes_strings)

}; // namespace workshop
