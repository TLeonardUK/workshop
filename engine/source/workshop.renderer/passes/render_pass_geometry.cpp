// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_geometry.h"
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

render_pass_geometry::render_pass_geometry()
{
    m_stats_triangles_rendered  = statistics_manager::get().find_or_create_channel("rendering/triangles_rendered",  1.0f, statistics_commit_point::end_of_render);
    m_stats_draw_calls          = statistics_manager::get().find_or_create_channel("rendering/draw_calls",          1.0f, statistics_commit_point::end_of_render);
    m_stats_drawn_instances     = statistics_manager::get().find_or_create_channel("rendering/drawn_instances",     1.0f, statistics_commit_point::end_of_render);
    m_stats_culled_instances    = statistics_manager::get().find_or_create_channel("rendering/culled_instances",    1.0f, statistics_commit_point::end_of_render);
}

void render_pass_geometry::generate(renderer& renderer, generated_state& state_output, render_view* view)
{
    std::vector<render_batch*> batches = renderer.get_batch_manager().get_batches(
        domain, 
        render_batch_usage::static_mesh);

    render_visibility_manager& visibility_manager = renderer.get_visibility_manager();

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

        // Select the technique to use.
        render_effect::technique* active_technique = technique;
        if (renderer.get_visualization_mode() == visualization_mode::wireframe && wireframe_technique)
        {
            active_technique = wireframe_technique;
        }

        // Setup initial state.
        list.set_pipeline(*active_technique->pipeline.get());
        list.set_render_targets(output.color_targets, output.depth_target);
        list.set_viewport(view->get_viewport());
        list.set_scissor(view->get_viewport());
        list.set_primitive_topology(ri_primitive::triangle_list);

        render_draw_flags view_draw_flags = view->get_draw_flags();

        // Draw each batch.
        for (size_t i = 0; i < batches.size(); i++)
        {
            render_batch* batch = batches[i];
            render_batch_key key = batch->get_key();
            const std::vector<render_batch_instance>& instances = batch->get_instances();

            model::mesh_info& mesh_info = key.model->meshes[key.mesh_index];
            asset_ptr<material>& mat = key.material;

            profile_gpu_marker(list, profile_colors::gpu_pass, "batch %zi / %zi", i, batches.size());
           
            // Generate the instance buffer for this batch.
            render_batch_instance_buffer* instance_buffer = batch->get_resource_cache().find_or_create_instance_buffer(get_cache_key(*view));
            size_t visible_instance_count = 0;

            for (size_t j = 0; j < instances.size(); j++)
            {
                const render_batch_instance& instance = instances[j];

                // Skip instance if its not visibile this frame.
                if (!visibility_manager.is_object_visibile(view->get_visibility_view_id(), instance.visibility_id))
                {
                    culled_instances++;
                    continue;
                }

                // Check geometry is drawn to this view.
                if (!instance.object->has_draw_flag(view_draw_flags))
                {
                    culled_instances++;
                    continue;
                }

                size_t table_index;
                size_t table_offset;
                instance.param_block->get_table(table_index, table_offset);

                instance_buffer->add(static_cast<uint32_t>(table_index), static_cast<uint32_t>(table_offset));
                visible_instance_count++;
                drawn_instances++;
            }
            instance_buffer->commit();

            // Nothing to render :(
            if (visible_instance_count == 0)
            {
                continue;
            }

            // Generate the vertex info buffer for this batch.
            std::size_t vertex_info_hash = reinterpret_cast<std::size_t>(get_cache_key(*view));
            hash_combine(vertex_info_hash, &instance_buffer->get_buffer());

            ri_param_block* vertex_info_param_block = batch->get_resource_cache().find_or_create_param_block((void*)vertex_info_hash, "vertex_info", {});

            size_t model_info_table_index;
            size_t model_info_table_offset;
            key.model->get_model_info_param_block().get_table(model_info_table_index, model_info_table_offset);

            size_t material_info_table_index;
            size_t material_info_table_offset;
            key.material->get_material_info_param_block()->get_table(material_info_table_index,  material_info_table_offset);

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
            list.draw(mesh_info.indices.size(), visible_instance_count);

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
