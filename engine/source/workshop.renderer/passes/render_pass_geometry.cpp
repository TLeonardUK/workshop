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

#include <Windows.h>

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
    //db_log(renderer, "--------- generate %zi ------------", renderer.get_frame_index());

    std::vector<render_batch*> batches = renderer.get_batch_manager().get_batches(
        domain, 
        render_batch_usage::static_mesh);

    std::atomic_size_t triangles_rendered = 0;
    std::atomic_size_t draw_calls = 0;
    std::atomic_size_t drawn_instances = 0;
    std::atomic_size_t culled_instances = 0;

    // Command list to transition output targets to the correct format.
    {
        ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
        list.open();

        {
            profile_gpu_marker(list, profile_colors::gpu_pass, "transition outputs", name.c_str());

            for (ri_texture_view texture : output.color_targets)
            {
                list.barrier(*texture.texture, ri_resource_state::initial, ri_resource_state::render_target);
            }
            if (output.depth_target.texture)
            {
                list.barrier(*output.depth_target.texture, ri_resource_state::initial, ri_resource_state::depth_write);
            }
        }

        list.close();
        state_output.graphics_command_lists.push_back(&list);
    }

    // Generate command lists in parallel for chunks of batches.
    size_t worker_count = task_scheduler::get().get_worker_count(task_queue::standard);     // NOTE: This trades CPU time for GPU time. The more command lists we create the less overlapping of work the gpu can do.
    size_t chunk_size = (size_t)std::ceilf((float)batches.size() / worker_count);
    std::atomic_size_t chunk_offset = 0;
    std::mutex output_list_mutex;

    auto callback = [&chunk_offset, chunk_size, &batches, &renderer, this, &view, &state_output, &triangles_rendered, &draw_calls, &drawn_instances, &culled_instances, &output_list_mutex](size_t i) mutable
    {
        size_t batch_start = chunk_offset.fetch_add(chunk_size);
        size_t batch_end = min(batches.size(), batch_start + chunk_size);

        if (batch_start >= batches.size())
        {
            return;
        }

        ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
        {
            list.open();
        
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
            render_visibility_manager& visibility_manager = renderer.get_visibility_manager();

            // Draw each batch.
            for (size_t i = batch_start; i < batch_end; i++)
            {
                render_batch* batch = batches[i];
                render_batch_key key = batch->get_key();
                const std::vector<render_batch_instance>& instances = batch->get_instances();

                model::mesh_info& mesh_info = key.model->meshes[key.mesh_index];
                asset_ptr<material>& mat = key.material;

                profile_gpu_marker(list, profile_colors::gpu_pass, "batch %s : %s", mesh_info.name.c_str(), mat->name.c_str());

                // Generate the instance buffer for this batch.
                size_t visible_instance_count = 0;
                render_batch_instance_buffer* instance_buffer = nullptr;
                {
                    instance_buffer = batch->get_resource_cache().find_or_create_instance_buffer(get_cache_key(*view));
                    for (size_t j = 0; j < instances.size(); j++)
                    {
                        const render_batch_instance& instance = instances[j];

                        // Skip instance if its not visibile this frame.
                        if (!visibility_manager.is_object_visibile(view->get_visibility_view_id(), instance.visibility_id))
                        {
                            culled_instances.fetch_add(1);
                            continue;
                        }

                        // Check geometry is drawn to this view.
                        if (!instance.object->has_draw_flag(view_draw_flags))
                        {
                            culled_instances.fetch_add(1);
                            continue;
                        }

                        size_t table_index;
                        size_t table_offset;
                        instance.param_block->get_table(table_index, table_offset);

                        instance_buffer->add(static_cast<uint32_t>(table_index), static_cast<uint32_t>(table_offset));
                        drawn_instances.fetch_and(1);
                        visible_instance_count++;
                    }
                    instance_buffer->commit();
                }

                // Nothing to render :(
                if (visible_instance_count == 0)
                {
                    continue;
                }

                {
                    // Generate the vertex info buffer for this batch.
                    std::size_t vertex_info_hash = reinterpret_cast<std::size_t>(get_cache_key(*view));
                    hash_combine(vertex_info_hash, &instance_buffer->get_buffer());

                    ri_param_block* vertex_info_param_block = batch->get_resource_cache().find_or_create_param_block((void*)vertex_info_hash, "vertex_info", {});

                    size_t model_info_table_index;
                    size_t model_info_table_offset;
                    key.model->get_model_info_param_block(key.mesh_index).get_table(model_info_table_index, model_info_table_offset);               

                    size_t material_info_table_index;
                    size_t material_info_table_offset;
                    key.material->get_material_info_param_block()->get_table(material_info_table_index, material_info_table_offset);                

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
                }

                triangles_rendered += mesh_info.indices.size() / 3;
                draw_calls++;
            }

            list.close();
        }

        // Output the completed list.
        {
            std::scoped_lock lock(output_list_mutex);
            state_output.graphics_command_lists.push_back(&list);
        }
    };
    
    // Run callback in parallel for each chunk of batches to handle.
    parallel_for("build geometry command lists", task_queue::standard, worker_count, callback, true);

    // Command list to transition output targets back to the original format
    {
        ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
        list.open();

        {
            profile_gpu_marker(list, profile_colors::gpu_pass, "transition outputs", name.c_str());

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
    }

    m_stats_triangles_rendered->submit(triangles_rendered);
    m_stats_draw_calls->submit(draw_calls);
    m_stats_culled_instances->submit(culled_instances);
    m_stats_drawn_instances->submit(drawn_instances);
}

}; // namespace ws
