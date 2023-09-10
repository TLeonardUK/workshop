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

        // Grab some default values used to contruct param blocks.
        ri_texture* default_black = renderer.get_default_texture(default_texture_type::black);
        ri_texture* default_white = renderer.get_default_texture(default_texture_type::white);
        ri_texture* default_grey = renderer.get_default_texture(default_texture_type::grey);
        ri_texture* default_normal = renderer.get_default_texture(default_texture_type::normal);
        ri_sampler* default_sampler_color = renderer.get_default_sampler(default_sampler_type::color);
        ri_sampler* default_sampler_normal = renderer.get_default_sampler(default_sampler_type::normal);

        // Draw each batch.
        for (size_t i = 0; i < batches.size(); i++)
        {
            render_batch* batch = batches[i];
            render_batch_key key = batch->get_key();
            const std::vector<render_batch_instance>& instances = batch->get_instances();

            model::mesh_info& mesh_info = key.model->meshes[key.mesh_index];
            asset_ptr<material>& mat = key.material;

            profile_gpu_marker(list, profile_colors::gpu_pass, "batch %zi / %zi", i, batches.size());

            // Generate the vertex buffer for this batch.
            model::vertex_buffer* vertex_buffer = key.model->find_or_create_vertex_buffer(active_technique->pipeline->get_create_params().vertex_layout);

            // Generate the geometry_info block for this material.  
            ri_param_block* geometry_info_param_block = batch->get_resource_cache().find_or_create_param_block(get_cache_key(*view), info_param_block_type.c_str(), [&mat, default_black, default_grey, default_white, default_normal, default_sampler_color, default_sampler_normal](ri_param_block& param_block) {

                // TODO: We should make this a callback to wherever is creating this geometry pass.
                // This is all currently unpleasently hard-coded.
                param_block.set("albedo_texture", *mat->get_texture("albedo_texture", default_black));
                param_block.set("opacity_texture", *mat->get_texture("opacity_texture", default_white));
                param_block.set("metallic_texture", *mat->get_texture("metallic_texture", default_black));
                param_block.set("roughness_texture", *mat->get_texture("roughness_texture", default_grey));
                param_block.set("normal_texture", *mat->get_texture("normal_texture", default_normal));
                param_block.set("skybox_texture", *mat->get_texture("skybox_texture", default_white));

                param_block.set("albedo_sampler", *mat->get_sampler("albedo_sampler", default_sampler_color));
                param_block.set("opacity_sampler", *mat->get_sampler("opacity_sampler", default_sampler_color));
                param_block.set("metallic_sampler", *mat->get_sampler("metallic_sampler", default_sampler_color));
                param_block.set("roughness_sampler", *mat->get_sampler("roughness_sampler", default_sampler_color));
                param_block.set("normal_sampler", *mat->get_sampler("normal_sampler", default_sampler_normal));
                param_block.set("skybox_sampler", *mat->get_sampler("skybox_sampler", default_sampler_color));

            });

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
            ri_param_block* vertex_info_param_block = batch->get_resource_cache().find_or_create_param_block(get_cache_key(*view), "vertex_info", {});
            vertex_info_param_block->set("vertex_buffer", *vertex_buffer->vertex_buffer.get());
            vertex_info_param_block->set("vertex_buffer_offset", 0u);
            vertex_info_param_block->set("instance_buffer", instance_buffer->get_buffer());        

            // Put together param block list to use.
            std::vector<ri_param_block*> blocks = bind_param_blocks(view->get_resource_cache());
            blocks.push_back(view->get_view_info_param_block());
            blocks.push_back(geometry_info_param_block);
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
