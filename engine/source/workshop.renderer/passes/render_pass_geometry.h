// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/passes/render_pass_graphics.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_output.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_param_block.h"

namespace ws {

class material;

// ================================================================================================
//  Render pass that draws a set of geometry visible from the given view. 
// ================================================================================================
class render_pass_geometry
    : public render_pass_graphics
{
private:
/*
    struct instance
    {
        matrix4 transform = matrix4::identity;
    };

    struct batch
    {
        material* material = nullptr;
        geometry_material* geometry_material = nullptr;
        model* model = nullptr;
        ri_buffer* index_buffer = nullptr;
        ri_param_block* geometry_info_param_block = nullptr;
        model::vertex_buffer* model_vertex_buffer = nullptr;
        size_t index_count = 0;

        std::vector<instance> instances;
    };
*/

public:

    virtual result<void> create_resources(renderer& renderer) override;
    virtual result<void> destroy_resources(renderer& renderer) override;

    virtual void generate(renderer& renderer, generated_state& output, render_view& view) override;

private:
//    std::vector<batch> calculate_batches(renderer& renderer);

private:
//    std::unique_ptr<ri_param_block> m_vertex_info_param_block;

};

}; // namespace ws
