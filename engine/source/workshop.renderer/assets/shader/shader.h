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
//  Defines the different types of rays that can be cast in the scene which determines
//  how their shaders are selected from the shader binding table.
// ================================================================================================
enum class ray_type
{
    // Traces primitive geometry in the scene and returns radiance 
    // values for them.
    primitive = 0,

    // Traces primitive geometry in the scene and returns depth 
    // values for them.
    occlusion = 1,

    COUNT
};

inline static const char* ray_type_strings[static_cast<int>(ray_type::COUNT)] = {
    "primitive",
    "occlusion"
};

DEFINE_ENUM_TO_STRING(ray_type, ray_type_strings)

// ================================================================================================
//  Defines the different maskes for TLAS instances that rays can intersect
// ================================================================================================
enum class ray_mask
{
    // Standard TLAS instances
    normal = 1,

    // TLAS instances which represent the sky.
    sky = 2,

    // TLAS instances marked as invisible.
    invisible = 4,

    all = normal | sky | invisible,
    all_visible = normal | sky,
};

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

    struct shader_stage
    {
        std::string file;
        std::string entry_point;
        std::vector<uint8_t> bytecode;
    };

    struct ray_hitgroup
    {
        std::string name;
        material_domain domain;
        ray_type type;
        std::array<shader_stage, static_cast<int>(ri_shader_stage::COUNT)> stages;
    };

    struct ray_missgroup
    {
        std::string name;
        ray_type type;
        shader_stage ray_miss_stage;
    };

    struct technique
    {
        std::string name;
        std::array<shader_stage, static_cast<int>(ri_shader_stage::COUNT)> stages;
        size_t render_state_index;
        size_t vertex_layout_index;
        size_t output_target_index;
        std::vector<size_t> param_block_indices;
        std::vector<ray_hitgroup> ray_hitgroups;
        std::vector<ray_missgroup> ray_missgroups;
        std::unordered_map<std::string, std::string> defines;
    };

public:
    shader(ri_interface& ri_interface, renderer& renderer);
    virtual ~shader();

    void swap(shader* other);

public:
    std::unordered_map<std::string, std::string> global_defines;
    std::vector<param_block> param_blocks;
    std::vector<render_state> render_states;
    std::vector<variation> variations;
    std::vector<vertex_layout> vertex_layouts;
    std::vector<output_target> output_targets;
    std::vector<effect> effects;
    std::vector<technique> techniques;
    std::vector<ray_hitgroup> ray_hitgroups;
    std::vector<ray_missgroup> ray_missgroups;

protected:

    std::unique_ptr<ri_pipeline> make_technique_pipeline(const technique& instance);

    virtual bool load_dependencies() override;

    void unregister_effects();

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;

};

}; // namespace workshop
