// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_imgui.h"
#include "workshop.renderer/render_world_state.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/common_types.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

void render_pass_imgui::generate(renderer& renderer, generated_state& state_output, render_view* view)
{
    ri_param_block* model_info_param_block = view->get_resource_cache().find_or_create_param_block(get_cache_key(*view), "model_info");
    model_info_param_block->set("index_size", (int)index_buffer->get_element_size());
    model_info_param_block->set("position_buffer", *position_buffer);
    model_info_param_block->set("uv0_buffer", *uv0_buffer);
    model_info_param_block->set("color0_buffer", *color0_buffer);

    size_t model_info_table_index;
    size_t model_info_table_offset;
    model_info_param_block->get_table(model_info_table_index, model_info_table_offset);

    ri_param_block* vertex_info_param_block = view->get_resource_cache().find_or_create_param_block(get_cache_key(*view), "vertex_info");
    vertex_info_param_block->set("model_info_table", (uint32_t)model_info_table_index);
    vertex_info_param_block->set("model_info_offset", (uint32_t)model_info_table_offset);
    vertex_info_param_block->set("material_info_table", 0);
    vertex_info_param_block->set("material_info_offset", 0);
    vertex_info_param_block->clear_buffer("instance_buffer");

    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        profile_gpu_marker(list, profile_colors::gpu_pass, "imgui");

        list.barrier(*output.color_targets[0].texture, ri_resource_state::initial, ri_resource_state::render_target);
        list.set_pipeline(*technique->pipeline.get());
        list.set_render_targets(output.color_targets, nullptr);
        list.set_viewport(view->get_viewport());
        list.set_scissor(view->get_viewport());
        list.set_primitive_topology(ri_primitive::triangle_list);
        list.set_index_buffer(*index_buffer);

        for (render_system_imgui::draw_command& cmd : draw_commands)
        {
            if ((int)cmd.scissor.width == 0 || (int)cmd.scissor.height == 0)
            {
                continue;
            }

            ri_texture* texture = static_cast<ri_texture*>(cmd.texture);
            if (texture == nullptr)
            {
                texture = default_texture;
            }

            ri_param_block* imgui_params = view->get_resource_cache().find_or_create_param_block(get_cache_key(*view), "imgui_params");
            imgui_params->set("color_texture", *texture);
            imgui_params->set("color_sampler", *renderer.get_default_sampler(default_sampler_type::color));
            imgui_params->set("projection_matrix", matrix4::orthographic(
                cmd.display_pos.x,
                cmd.display_pos.x + cmd.display_size.x,
                cmd.display_pos.y,
                cmd.display_pos.y + cmd.display_size.y,
                0.0f,
                1.0f
            ));
            
            list.set_param_blocks({ vertex_info_param_block, imgui_params });
            list.set_scissor(recti((int)cmd.scissor.x, (int)cmd.scissor.y, (int)cmd.scissor.width, (int)cmd.scissor.height));
            list.draw(cmd.count, 1, cmd.offset);
        }

        list.barrier(*output.color_targets[0].texture, ri_resource_state::render_target, ri_resource_state::initial);
        
    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws
