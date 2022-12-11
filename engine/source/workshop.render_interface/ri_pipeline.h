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

        std::array<stage, static_cast<int>(ri_shader_stage::COUNT)> stages;
        ri_pipeline_render_state render_state;
        std::vector<ri_param_block_archetype*> param_block_archetypes;
        ri_data_layout vertex_layout;

        std::vector<ri_data_type> bindless_arrays_types;

        std::vector<ri_texture_format> color_formats;
        ri_texture_format depth_format;

    };

public:

    virtual create_params get_create_params() = 0;

};

}; // namespace ws
