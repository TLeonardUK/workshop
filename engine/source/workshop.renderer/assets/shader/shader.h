// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset.h"
#include "workshop.core/containers/string.h"

#include "workshop.render_interface/ri_types.h"
#include "workshop.render_interface/ri_pipeline.h"

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_effect_manager.h"

#include <array>
#include <unordered_map>

namespace ws {

class asset;
class ri_interface;
class renderer;

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
        std::string name;
        ri_data_scope scope;
        ri_data_layout layout;

        std::unique_ptr<ri_param_block_archetype> archetype;

        render_param_block_manager::param_block_archetype_id renderer_id;
    };

    struct render_state
    {
        std::string name;
        ri_pipeline_render_state state;
    };

    struct variation
    {
        std::string name;
        std::vector<std::string> values;
    };

    struct vertex_layout
    {
        std::string name;
        ri_data_layout layout;
    };

    struct output_target
    {
        std::string name;
        std::vector<ri_texture_format> color;
        ri_texture_format depth;
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

        render_effect_manager::effect_id renderer_id;
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
        std::array<stage, static_cast<int>(ri_shader_stage::COUNT)> stages;
        size_t render_state_index;
        size_t vertex_layout_index;
        size_t output_target_index;
        std::vector<size_t> param_block_indices;
        std::unordered_map<std::string, std::string> defines;
    };

public:
    shader(ri_interface& ri_interface, renderer& renderer);
    virtual ~shader();

public:
    std::vector<param_block> param_blocks;
    std::vector<render_state> render_states;
    std::vector<variation> variations;
    std::vector<vertex_layout> vertex_layouts;
    std::vector<output_target> output_targets;
    std::vector<effect> effects;
    std::vector<technique> techniques;

protected:

    std::unique_ptr<ri_pipeline> make_technique_pipeline(const technique& instance);

    virtual bool post_load() override;

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;

};

}; // namespace workshop
