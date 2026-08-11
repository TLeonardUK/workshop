// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_gpu_logs.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/passes/render_pass_callback.h"
#include "workshop.renderer/passes/render_pass_primitives.h"
#include "workshop.renderer/passes/render_pass_readback_buffer.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"

#include "workshop.core/math/matrix4.h"
#include "workshop.core/perf/profile.h"

#include "workshop.render_interface/ri_interface.h"

namespace ws {

render_system_gpu_logs::render_system_gpu_logs(renderer& render)
    : render_system(render, "gpu_logs")
{
}

void render_system_gpu_logs::register_init(init_list& list)
{
    list.add_step(
        "Resolve Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );
}

result<void> render_system_gpu_logs::create_resources()
{
    for (int i = 0; i < renderer::k_frame_depth; i++)
    {
        m_frame_buffers[i] = std::make_unique<buffer>();

        ri_buffer::create_params buffer_params;
        buffer_params.element_count = k_output_buffer_size;
        buffer_params.element_size = 1;
        buffer_params.usage = ri_buffer_usage::generic;
        m_frame_buffers[i]->m_output_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "gpu logs write buffer");

        buffer_params.usage = ri_buffer_usage::readback;
        m_frame_buffers[i]->m_cpu_output_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "gpu logs write buffer [cpu]");

        buffer_params.element_count = 1;
        buffer_params.element_size = 4;
        buffer_params.usage = ri_buffer_usage::generic;
        m_frame_buffers[i]->m_output_buffer_write_offset_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "gpu logs write offset buffer");

        buffer_params.usage = ri_buffer_usage::readback;
        m_frame_buffers[i]->m_cpu_output_buffer_write_offset_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "gpu logs write offset buffer [cpu]");

        m_frame_buffers[i]->m_debug_param_block = m_renderer.get_param_block_manager().create_param_block("debug_info");
        m_frame_buffers[i]->m_debug_param_block->set("debug_output_buffer_size"_sh, (uint32_t)k_output_buffer_size);
        m_frame_buffers[i]->m_debug_param_block->set("debug_output_buffer_write_offset"_sh, *m_frame_buffers[i]->m_output_buffer_write_offset_buffer, true);
        m_frame_buffers[i]->m_debug_param_block->set("debug_output_buffer"_sh, *m_frame_buffers[i]->m_output_buffer, true);
    }

    return true;
}

result<void> render_system_gpu_logs::destroy_resources()
{
    for (int i = 0; i < renderer::k_frame_depth; i++)
    {
        m_frame_buffers[i] = std::make_unique<buffer>();
    }

    return true;
}

ri_param_block* render_system_gpu_logs::get_param_block()
{
    size_t index = m_renderer.get_frame_index() % renderer::k_frame_depth;
    return m_frame_buffers[index]->m_debug_param_block.get();
}

void render_system_gpu_logs::build_post_graph(render_graph& graph, const render_world_state& state)
{
    size_t index = m_renderer.get_frame_index() % renderer::k_frame_depth;

    // TODO: add a pre_graph to zero out the write offset.

    // TODO: Could just put this in host coherent memory?

    // Read back the output buffer.
    std::unique_ptr<render_pass_readback_buffer> readback_pass = std::make_unique<render_pass_readback_buffer>();
    readback_pass->name = "readback gpu logs";
    readback_pass->system = this;
    readback_pass->source_buffer = m_frame_buffers[index]->m_output_buffer.get();
    readback_pass->destination_buffer = m_frame_buffers[index]->m_cpu_output_buffer.get(); 
    graph.add_node(std::move(readback_pass));

    // Read back the output buffer.
    readback_pass = std::make_unique<render_pass_readback_buffer>();
    readback_pass->name = "readback gpu logs offset";
    readback_pass->system = this;
    readback_pass->source_buffer = m_frame_buffers[index]->m_output_buffer_write_offset_buffer.get();
    readback_pass->destination_buffer = m_frame_buffers[index]->m_cpu_output_buffer_write_offset_buffer.get();
    readback_pass->clear_source = true;
    graph.add_node(std::move(readback_pass));

    m_renderer.queue_frame_complete_callback([i = index, data_length_buffer = m_frame_buffers[index]->m_cpu_output_buffer_write_offset_buffer.get(), data_buffer = m_frame_buffers[index]->m_cpu_output_buffer.get()]()
    {
        uint32_t* length_ptr = (uint32_t*)data_length_buffer->map(0, 4);
        uint32_t length = std::min((uint32_t)k_output_buffer_size, *length_ptr);
        db_log(core, "Reading %i - %i\n", i, length);
        if (length > 0)
        {
            uint8_t* data_ptr = (uint8_t*)data_buffer->map(0, length);
            uint8_t* start_ptr = data_ptr;

            while (std::distance(start_ptr, data_ptr) < length)
            {
                // Decode the header.
                uint32_t signature = *((uint32_t*)data_ptr);
                data_ptr += 4;

                if (signature != 0xBEEFCAFE)
                {
                    break;
                }

                uint32_t type = *((uint32_t*)data_ptr);
                data_ptr += 4;
                uint32_t size = *((uint32_t*)data_ptr);
                data_ptr += 4;
                uint32_t category = *((uint32_t*)data_ptr);
                data_ptr += 4;

                // Decode the format.

                // Decode the arguments.
            }

            // Consume the logs.
            db_log(core, "Logs available: %i..\n", length);

            data_buffer->unmap(data_ptr);
        }

        data_length_buffer->unmap(length_ptr);
    });
}

void render_system_gpu_logs::step(const render_world_state& state)
{
    ri_interface& ri = m_renderer.get_render_interface();
}

}; // namespace ws
