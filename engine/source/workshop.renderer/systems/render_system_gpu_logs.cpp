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

        buffer_params.usage = ri_buffer_usage::generic;
        m_frame_buffers[i]->m_output_buffer_write_count_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "gpu logs write count buffer");

        buffer_params.usage = ri_buffer_usage::readback;
        m_frame_buffers[i]->m_cpu_output_buffer_write_count_buffer = m_renderer.get_render_interface().create_buffer(buffer_params, "gpu logs write count buffer [cpu]");

        m_frame_buffers[i]->m_debug_param_block = m_renderer.get_param_block_manager().create_param_block("debug_info");
        m_frame_buffers[i]->m_debug_param_block->set("debug_output_buffer_size"_sh, (uint32_t)k_output_buffer_size);
        m_frame_buffers[i]->m_debug_param_block->set("debug_output_buffer_write_offset"_sh, *m_frame_buffers[i]->m_output_buffer_write_offset_buffer, true);
        m_frame_buffers[i]->m_debug_param_block->set("debug_output_buffer_write_count"_sh, *m_frame_buffers[i]->m_output_buffer_write_count_buffer, true);
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

void render_system_gpu_logs::emit_message(render_gpu_log_type type, int category, const char* format, const std::vector<uint32_t>& args)
{
    // Format the message.
    std::string message = "";
    const char* format_ptr = format;
    int arg_index = 0;
    while (format_ptr[0] != '\0')
    {
        char c = format_ptr[0];
        if (c == '%')
        {
            format_ptr++;
            if (format_ptr[0] == '%')
            {
                message += "%";
                format_ptr++;
            }
            else
            {
                bool is_float = false;

                std::string format_arg = "%";
                while (format_ptr[0] != '\0')
                {
                    c = format_ptr[0];
                    format_arg += c;
                    format_ptr++;

                    if (c == 'f' || c == 'F' ||
                        c == 'e' || c == 'E' ||
                        c == 'g' || c == 'G' ||
                        c == 'a' || c == 'A')
                    {
                        is_float = true;
                    }

                    // Final character in format string.
                    if (c == 'd' || c == 'i' ||
                        c == 'u' || c == 'x' ||
                        c == 'X' || c == 'f' ||
                        c == 'F' || c == 'e' ||
                        c == 'E' || c == 'g' ||
                        c == 'G' || c == 'a' ||
                        c == 'A' || c == 'c' ||
                        c == 's' || c == 'p')
                    {
                        break;
                    }
                }

                char buffer[128];
                uint32_t arg = args[arg_index++];
                if (is_float)
                {
                    snprintf(buffer, sizeof(buffer), format_arg.c_str(), *((float*)&arg));
                }
                else
                {
                    snprintf(buffer, sizeof(buffer), format_arg.c_str(), arg);
                }
                message += buffer;
            }
        }
        else
        {
            format_ptr++;
            message += c;
        }
    }

    // Emit the message.
    switch (type)
    {
        case render_gpu_log_type::verbose:
        {
            ws::log_handler::static_write(ws::log_level::verbose, (log_source)category, "%s", message.c_str());
            break;
        }
        case render_gpu_log_type::log:
        {
            ws::log_handler::static_write(ws::log_level::log, (log_source)category, "%s", message.c_str());
            break;
        }
        case render_gpu_log_type::success:
        {
            ws::log_handler::static_write(ws::log_level::success, (log_source)category, "%s", message.c_str());
            break;
        }
        case render_gpu_log_type::warning:
        {
            ws::log_handler::static_write(ws::log_level::warning, (log_source)category, "%s", message.c_str());
            break;
        }
        case render_gpu_log_type::error:
        {
            ws::log_handler::static_write(ws::log_level::error, (log_source)category, "%s", message.c_str());
            break;
        }
        case render_gpu_log_type::fatal:
        {
            ws::log_handler::static_write(ws::log_level::fatal, (log_source)category, "%s", message.c_str());
            break;
        }
        case render_gpu_log_type::assert:
        {
            db_assert_message(false, "%s", message.c_str());
            break;
        }
        default:
        {
            db_assert_message(false, "Recieved unexpected or unimplemented log type '%i' from GPU.", (int)type);
            break;
        }
    }
}

void render_system_gpu_logs::build_post_graph(render_graph& graph, const render_world_state& state)
{
    size_t index = m_renderer.get_frame_index() % renderer::k_frame_depth;

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

    // Read back the count buffer.
    readback_pass = std::make_unique<render_pass_readback_buffer>();
    readback_pass->name = "readback gpu logs count";
    readback_pass->system = this;
    readback_pass->source_buffer = m_frame_buffers[index]->m_output_buffer_write_count_buffer.get();
    readback_pass->destination_buffer = m_frame_buffers[index]->m_cpu_output_buffer_write_count_buffer.get();
    readback_pass->clear_source = true;
    graph.add_node(std::move(readback_pass));

    m_renderer.queue_frame_complete_callback([i = index, 
        data_offset_buffer = m_frame_buffers[index]->m_cpu_output_buffer_write_offset_buffer.get(), 
        data_count_buffer = m_frame_buffers[index]->m_cpu_output_buffer_write_count_buffer.get(),
        data_buffer = m_frame_buffers[index]->m_cpu_output_buffer.get()]()
    {
        uint32_t* length_ptr = (uint32_t*)data_offset_buffer->map(0, 4);
        uint32_t length = std::min((uint32_t)k_output_buffer_size, *length_ptr);

        uint32_t* count_ptr = (uint32_t*)data_count_buffer->map(0, 4);
        uint32_t count = *count_ptr;

        if (count > 0)
        {
            if (*length_ptr > (uint32_t)k_output_buffer_size)
            {
                db_warning(gpu, "GPU log output is larger than available buffer, output will be truncated.");
            }

            uint8_t* data_ptr = (uint8_t*)data_buffer->map(0, length);
            uint8_t* start_ptr = data_ptr;

            for (size_t i = 0; i < count; i++)
            {
                // Decode the header
                uint32_t type = *((uint32_t*)data_ptr);
                data_ptr += 4;
                uint32_t size = *((uint32_t*)data_ptr);
                data_ptr += 4;
                uint32_t category = *((uint32_t*)data_ptr);
                data_ptr += 4;
                uint32_t arg_count = *((uint32_t*)data_ptr);
                data_ptr += 4;

                // Decode the format.
                std::string format = "";
                std::vector<uint32_t> args;
                while (true)
                {
                    uint32_t dword = *((uint32_t*)data_ptr);
                    data_ptr += 4;

                    uint8_t byte1 = (dword) & 0xFF;
                    uint8_t byte2 = (dword >> 8) & 0xFF;
                    uint8_t byte3 = (dword >> 16) & 0xFF;
                    uint8_t byte4 = (dword >> 24) & 0xFF;

                    if (byte1 != 0)
                    {
                        format += (char)byte1;
                    }
                    if (byte2 != 0)
                    {
                        format += (char)byte2;
                    }
                    if (byte3 != 0)
                    {
                        format += (char)byte3;
                    }
                    if (byte4 != 0)
                    {
                        format += (char)byte4;
                    }

                    // Break once we've found null terminator.
                    if (byte1 == 0 || byte2 == 0 || byte3 == 0 || byte4 == 0)
                    {
                        break;
                    }
                }

                // Decode the arguments.
                for (size_t j = 0; j < arg_count; j++)
                {
                    uint32_t arg = *((uint32_t*)data_ptr);
                    data_ptr += 4;
                    args.push_back(arg);
                }

                emit_message((render_gpu_log_type)type, category, format.c_str(), args);
            }

            data_buffer->unmap(data_ptr);
        }

        data_count_buffer->unmap(count_ptr);
        data_offset_buffer->unmap(length_ptr);
    });
}

void render_system_gpu_logs::step(const render_world_state& state)
{
    ri_interface& ri = m_renderer.get_render_interface();
}

}; // namespace ws
