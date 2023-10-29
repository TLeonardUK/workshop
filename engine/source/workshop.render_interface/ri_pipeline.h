// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"

#include <array>

namespace ws {

class ri_param_block_archetype;

// ================================================================================================
//  State of the GPU pipeline at the point a draw call is dispatched.
// ================================================================================================
class ri_pipeline
{
public:

    struct create_params
    {
        struct stage
        {
            std::string file;
            std::string entry_point;
            std::vector<uint8_t> bytecode;
        };

        struct ray_hitgroup
        {
            const char* name;
            size_t domain;
            size_t type;
            std::array<stage, static_cast<int>(ri_shader_stage::COUNT)> stages;
        };

        struct ray_missgroup
        {
            const char* name;
            size_t type;
            stage ray_miss_stage;
        };

        std::array<stage, static_cast<int>(ri_shader_stage::COUNT)> stages;
        ri_pipeline_render_state render_state;
        std::vector<ri_param_block_archetype*> param_block_archetypes;
        ri_data_layout vertex_layout;

        std::vector<ray_hitgroup> ray_hitgroups;
        std::vector<ray_missgroup> ray_missgroups;

        std::vector<ri_descriptor_table> descriptor_tables;

        std::vector<ri_texture_format> color_formats;
        ri_texture_format depth_format;

    };

public:
    virtual ~ri_pipeline() {}

    virtual const create_params& get_create_params() = 0;

};

}; // namespace ws
