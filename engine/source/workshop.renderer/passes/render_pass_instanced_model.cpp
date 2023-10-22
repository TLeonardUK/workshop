// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_instanced_model.h"
#include "workshop.renderer/render_world_state.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/common_types.h"
#include "workshop.core/geometry/geometry.h"
#include "workshop.core/statistics/statistics_manager.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/common_types.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

render_pass_instanced_model::render_pass_instanced_model()
{
    m_stats_triangles_rendered  = statistics_manager::get().find_or_create_channel("rendering/triangles_rendered",  1.0f, statistics_commit_point::end_of_render);
    m_stats_draw_calls          = statistics_manager::get().find_or_create_channel("rendering/draw_calls",          1.0f, statistics_commit_point::end_of_render);
    m_stats_drawn_instances     = statistics_manager::get().find_or_create_channel("rendering/drawn_instances",     1.0f, statistics_commit_point::end_of_render);
    m_stats_culled_instances    = statistics_manager::get().find_or_create_channel("rendering/culled_instances",    1.0f, statistics_commit_point::end_of_render);
}

void render_pass_instanced_model::generate(renderer& renderer, generated_state& state_output, render_view* view)
{
    if (!render_model.is_loaded() || instances.empty())
    {
        return;
    }

    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();

    size_t triangles_rendered = 0;
    size_t draw_calls = 0;
    size_t drawn_instances = 0;
    size_t culled_instances = 0;
    
    {
        profile_gpu_marker(list, profile_colors::gpu_pass, "%s", name.c_str());

        // Transition targets to the relevant state.
        for (ri_texture_view texture : output.color_targets)
        {
            list.barrier(*texture.texture, ri_resource_state::initial, ri_resource_state::render_target);
        }
        if (output.depth_target.texture)
        {
            list.barrier(*output.depth_target.texture, ri_resource_state::initial, ri_resource_state::depth_write);
        }

        // Setup initial state.
        list.set_pipeline(*technique->pipeline.get());
        list.set_render_targets(output.color_targets, output.depth_target);
        list.set_viewport(view->get_viewport());
        list.set_scissor(view->get_viewport());
        list.set_primitive_topology(ri_primitive::triangle_list);

        // Draw each mesh for the model.
        model* model_instance = render_model.get();

        for (size_t i = 0; i < model_instance->meshes.size(); i++)
        {
            profile_gpu_marker(list, profile_colors::gpu_pass, "mesh %zi / %zi", i, model_instance->meshes.size());

            model::mesh_info& mesh_info = model_instance->meshes[i];
            model::material_info& material_info = model_instance->materials[mesh_info.material_index];

            // Generate the instance buffer for this batch.
            render_batch_instance_buffer* instance_buffer = view->get_resource_cache().find_or_create_instance_buffer(get_cache_key(*view));
            for (size_t j = 0; j < instances.size(); j++)
            {
                ri_param_block* instance = instances[j];
                
                size_t table_index;
                size_t table_offset;
                instance->get_table(table_index, table_offset);

                instance_buffer->add(static_cast<uint32_t>(table_index), static_cast<uint32_t>(table_offset));
            }
            instance_buffer->commit();

            // Generate the vertex info buffer for this batch.
            ri_param_block* vertex_info_param_block = view->get_resource_cache().find_or_create_param_block(get_cache_key(*view), "vertex_info", {});

            size_t model_info_table_index;
            size_t model_info_table_offset;
            model_instance->get_model_info_param_block().get_table(model_info_table_index, model_info_table_offset);

            size_t material_info_table_index;
            size_t material_info_table_offset;
            material_info.material->get_material_info_param_block()->get_table(material_info_table_index, material_info_table_offset);

            vertex_info_param_block->set("model_info_table", (uint32_t)model_info_table_index);
            vertex_info_param_block->set("model_info_offset", (uint32_t)model_info_table_offset);
            vertex_info_param_block->set("material_info_table", (uint32_t)material_info_table_index);
            vertex_info_param_block->set("material_info_offset", (uint32_t)material_info_table_offset);
            vertex_info_param_block->set("instance_buffer", instance_buffer->get_buffer());        

            // Put together param block list to use.
            std::vector<ri_param_block*> blocks = bind_param_blocks(view->get_resource_cache());
            blocks.push_back(view->get_view_info_param_block());
            blocks.push_back(vertex_info_param_block);
            list.set_param_blocks(blocks);

            // Draw everything!
            list.set_index_buffer(*mesh_info.index_buffer);        
            list.draw(mesh_info.indices.size(), instances.size());

            triangles_rendered += mesh_info.indices.size() / 3;
            draw_calls++;
        }

        // Transition targets back to initial state.
        for (ri_texture_view texture : output.color_targets)
        {
            list.barrier(*texture.texture, ri_resource_state::render_target, ri_resource_state::initial);
        }
        if (output.depth_target.texture)
        {
            list.barrier(*output.depth_target.texture, ri_resource_state::depth_write, ri_resource_state::initial);
        }
    }

    list.close();
    state_output.graphics_command_lists.push_back(&list);

    m_stats_triangles_rendered->submit(triangles_rendered);
    m_stats_draw_calls->submit(draw_calls);
    m_stats_culled_instances->submit(culled_instances);
    m_stats_drawn_instances->submit(drawn_instances);
}

}; // namespace ws
