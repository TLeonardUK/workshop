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
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/common_types.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

result<void> render_pass_geometry::create_resources(renderer& renderer)
{
    return true;
}

result<void> render_pass_geometry::destroy_resources(renderer& renderer)
{
    return true;
}

void render_pass_geometry::generate(renderer& renderer, generated_state& state_output, render_view& view)
{
    std::vector<render_batch*> batches = renderer.get_batch_manager().get_batches(
        material_domain::opaque, 
        render_batch_usage::static_mesh);

    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    
    {
        profile_gpu_marker(list, profile_colors::gpu_pass, "%s", name.c_str());

        // Transition targets to the relevant state.
        for (ri_texture* texture : output.color_targets)
        {
            list.barrier(*texture, ri_resource_state::initial, ri_resource_state::render_target);
        }
        if (output.depth_target)
        {
            list.barrier(*output.depth_target, ri_resource_state::initial, ri_resource_state::depth_write);
        }

        // Setup initial state.
        list.set_pipeline(*technique->pipeline.get());
        list.set_render_targets(output.color_targets, output.depth_target);
        list.set_viewport(view.get_viewport());
        list.set_scissor(view.get_viewport());
        list.set_primitive_topology(ri_primitive::triangle_list);

        // Grab some default values used to contruct param blocks.
        ri_texture* default_black = renderer.get_default_texture(default_texture_type::black);
        ri_texture* default_normal = renderer.get_default_texture(default_texture_type::normal);
        ri_sampler* default_sampler_color = renderer.get_default_sampler(default_sampler_type::color);
        ri_sampler* default_sampler_normal = renderer.get_default_sampler(default_sampler_type::normal);

        // Draw each batch.
        for (size_t i = 0; i < batches.size(); i++)
        {
            render_batch* batch = batches[i];
            render_batch_key key = batch->get_key();
            const std::vector<render_batch_instance>& instances = batch->get_instances();

            // Skip batch if resources have not finished loading for it.
            if (!key.model.is_loaded())
            {
                continue;
            }
        
            model::material_info& material_info = key.model->materials[key.material_index];
            if (!material_info.material.is_loaded())
            {
                continue;
            }

            profile_gpu_marker(list, profile_colors::gpu_pass, "batch %zi / %zi", i, batches.size());

            // Generate the vertex buffer for this batch.
            model::vertex_buffer* vertex_buffer = key.model->find_or_create_vertex_buffer(technique->pipeline->get_create_params().vertex_layout);

            // Generate the geometry_info block for this material.  
            ri_param_block* geometry_info_param_block = batch->find_or_create_param_block(this, "geometry_info",  [&material_info, default_black, default_normal, default_sampler_color, default_sampler_normal](ri_param_block& param_block) {
                param_block.set("albedo_texture", *material_info.material->get_texture("albedo_texture", default_black));
                param_block.set("metallic_texture", *material_info.material->get_texture("metallic_texture", default_black));
                param_block.set("roughness_texture", *material_info.material->get_texture("roughness_texture", default_black));
                param_block.set("normal_texture", *material_info.material->get_texture("normal_texture", default_normal));
                param_block.set("albedo_sampler", *material_info.material->get_sampler("albedo_sampler", default_sampler_color));
                param_block.set("metallic_sampler", *material_info.material->get_sampler("metallic_sampler", default_sampler_color));
                param_block.set("roughness_sampler", *material_info.material->get_sampler("roughness_sampler", default_sampler_color));
                param_block.set("normal_sampler", *material_info.material->get_sampler("normal_sampler", default_sampler_normal));
            });

            // Generate the instance buffer for this batch.
            render_batch_instance_buffer* instance_buffer = batch->find_or_create_instance_buffer(this);
            for (size_t j = 0; j < instances.size(); j++)
            {
                const render_batch_instance& instance = instances[j];

                // TODO: Do culling

                size_t table_index;
                size_t table_offset;

                instance.param_block->get_table(table_index, table_offset);

                instance_buffer->add(static_cast<uint32_t>(table_index), static_cast<uint32_t>(table_offset));
            }
            instance_buffer->commit();

            // Generate the vertex info buffer for this batch.
            ri_param_block* vertex_info_param_block = batch->find_or_create_param_block(this, "vertex_info", {});
            vertex_info_param_block->set("vertex_buffer", *vertex_buffer->vertex_buffer.get());
            vertex_info_param_block->set("vertex_buffer_offset", 0u);
            vertex_info_param_block->set("instance_buffer", instance_buffer->get_buffer());        

            // Put together param block list to use.
            std::vector<ri_param_block*> blocks = param_blocks;
            blocks.push_back(view.get_view_info_param_block());
            blocks.push_back(geometry_info_param_block);
            blocks.push_back(vertex_info_param_block);
            list.set_param_blocks(blocks);

            // Draw everything!
            list.set_index_buffer(*material_info.index_buffer);        
            list.draw(material_info.indices.size(), instances.size());
        }

        // Transition targets back to initial state.
        for (ri_texture* texture : output.color_targets)
        {
            list.barrier(*texture, ri_resource_state::render_target, ri_resource_state::initial);
        }
        if (output.depth_target)
        {
            list.barrier(*output.depth_target, ri_resource_state::depth_write, ri_resource_state::initial);
        }
    }

    list.close();
    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws
