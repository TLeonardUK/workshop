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
    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        recti display_rect = recti(0, 0, (int)output.color_targets[0].get_width(), (int)output.color_targets[0].get_height());

        list.barrier(*output.color_targets[0].texture, ri_resource_state::initial, ri_resource_state::render_target);
        list.set_pipeline(*technique->pipeline.get());
        list.set_render_targets(output.color_targets, nullptr);
        list.set_viewport(display_rect);
        list.set_scissor(display_rect);
        list.set_primitive_topology(ri_primitive::triangle_list);
        list.set_index_buffer(*index_buffer);

        size_t cmd_index = 0;
        for (render_system_imgui::draw_command& cmd : draw_commands)
        {
            if ((int)cmd.scissor.width == 0 || (int)cmd.scissor.height == 0)
            {
                continue;
            }

            bool correct_srgb = false;
            ri_texture* texture = static_cast<ri_texture*>(cmd.texture);
            if (texture == nullptr)
            {
                texture = default_texture;
                correct_srgb = true;
            }

            ri_param_block* imgui_params = cmd.param_block;
            imgui_params->set("color_texture"_sh, *texture);
            imgui_params->set("color_sampler"_sh, *renderer.get_default_sampler(default_sampler_type::color));
            imgui_params->set("projection_matrix"_sh, matrix4::orthographic(
                cmd.display_pos.x,
                cmd.display_pos.x + cmd.display_size.x,
                cmd.display_pos.y,
                cmd.display_pos.y + cmd.display_size.y,
                0.0f,
                1.0f
            ));
            imgui_params->set("correct_srgb"_sh, correct_srgb);

            std::vector<ri_param_block*> blocks = param_blocks;
            blocks.push_back(imgui_params);

            list.set_param_blocks(blocks);
            list.set_scissor(recti((int)cmd.scissor.x, (int)cmd.scissor.y, (int)cmd.scissor.width, (int)cmd.scissor.height));
            list.draw(cmd.count, 1, cmd.offset);
        }

        list.barrier(*output.color_targets[0].texture, ri_resource_state::render_target, ri_resource_state::initial);
        
    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws
