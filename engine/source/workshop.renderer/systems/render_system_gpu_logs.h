// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_system.h"
#include "workshop.renderer/render_pass.h"

#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_buffer.h"

namespace ws {

class renderer;
class render_pass_callback;
class render_view;
class ri_param_block;

// Value written by the gpu to the readback buffer for log messages/etc so the cpu can pick them up.
enum class render_gpu_log_type
{
    verbose,
    log,
    success,
    warning,
    error,
    fatal,
    assert
};

// ================================================================================================
//  Reads back logs the gpu has emitted
// ================================================================================================
class render_system_gpu_logs
    : public render_system
{
public:
    render_system_gpu_logs(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void step(const render_world_state& state) override;
    virtual void build_post_graph(render_graph& graph, const render_world_state& state) override;

    ri_param_block* get_param_block();

private:
    result<void> create_resources();
    result<void> destroy_resources();

    static void emit_message(render_gpu_log_type type, int category, const char* format, const std::vector<uint32_t>& args);

private:
    static const size_t k_output_buffer_size = 32 * 1024;

    struct buffer
    {
        std::unique_ptr<ri_buffer> m_output_buffer_write_offset_buffer;
        std::unique_ptr<ri_buffer> m_output_buffer_write_count_buffer;
        std::unique_ptr<ri_buffer> m_output_buffer;

        std::unique_ptr<ri_buffer> m_cpu_output_buffer_write_offset_buffer;
        std::unique_ptr<ri_buffer> m_cpu_output_buffer_write_count_buffer;
        std::unique_ptr<ri_buffer> m_cpu_output_buffer;

        std::unique_ptr<ri_param_block> m_debug_param_block;
    };

    std::array<std::unique_ptr<buffer>, renderer::k_frame_depth> m_frame_buffers;

};

}; // namespace ws
