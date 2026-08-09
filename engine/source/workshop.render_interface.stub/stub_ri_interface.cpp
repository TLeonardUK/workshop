// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.stub/stub_ri_interface.h"
#include "workshop.render_interface.stub/stub_ri_swapchain.h"
#include "workshop.render_interface.stub/stub_ri_fence.h"
#include "workshop.render_interface.stub/stub_ri_shader_compiler.h"
#include "workshop.render_interface.stub/stub_ri_texture_compiler.h"
#include "workshop.render_interface.stub/stub_ri_pipeline.h"
#include "workshop.render_interface.stub/stub_ri_param_block_archetype.h"
#include "workshop.render_interface.stub/stub_ri_texture.h"
#include "workshop.render_interface.stub/stub_ri_sampler.h"
#include "workshop.render_interface.stub/stub_ri_buffer.h"
#include "workshop.render_interface.stub/stub_ri_layout_factory.h"
#include "workshop.render_interface.stub/stub_ri_query.h"
#include "workshop.render_interface.stub/stub_ri_raytracing_blas.h"
#include "workshop.render_interface.stub/stub_ri_raytracing_tlas.h"
#include "workshop.render_interface.stub/stub_ri_staging_buffer.h"

namespace ws {

stub_render_interface::stub_render_interface(size_t ray_type_count, size_t ray_domain_count)
    : m_ray_type_count(ray_type_count)
    , m_ray_domain_count(ray_domain_count)
{
}

stub_render_interface::~stub_render_interface() = default;

void stub_render_interface::register_init(init_list& list)
{
}

void stub_render_interface::begin_frame()
{
}

void stub_render_interface::end_frame()
{
}

void stub_render_interface::flush_uploads()
{
}

std::unique_ptr<ri_swapchain> stub_render_interface::create_swapchain(window& for_window, const char* debug_name)
{
    return std::make_unique<stub_ri_swapchain>(for_window, debug_name);
}

std::unique_ptr<ri_fence> stub_render_interface::create_fence(const char* debug_name)
{
    return std::make_unique<stub_ri_fence>();
}

std::unique_ptr<ri_shader_compiler> stub_render_interface::create_shader_compiler()
{
    return std::make_unique<stub_ri_shader_compiler>();
}

std::unique_ptr<ri_texture_compiler> stub_render_interface::create_texture_compiler()
{
    return std::make_unique<stub_ri_texture_compiler>();
}

std::unique_ptr<ri_pipeline> stub_render_interface::create_pipeline(const ri_pipeline::create_params& params, const char* debug_name)
{
    return std::make_unique<stub_ri_pipeline>(params, debug_name);
}

std::unique_ptr<ri_param_block_archetype> stub_render_interface::create_param_block_archetype(const ri_param_block_archetype::create_params& params, const char* debug_name)
{
    return std::make_unique<stub_ri_param_block_archetype>(params, debug_name);
}

std::unique_ptr<ri_texture> stub_render_interface::create_texture(const ri_texture::create_params& params, const char* debug_name)
{
    return std::make_unique<stub_ri_texture>(params, debug_name);
}

std::unique_ptr<ri_sampler> stub_render_interface::create_sampler(const ri_sampler::create_params& params, const char* debug_name)
{
    return std::make_unique<stub_ri_sampler>(params, debug_name);
}

std::unique_ptr<ri_buffer> stub_render_interface::create_buffer(const ri_buffer::create_params& params, const char* debug_name)
{
    return std::make_unique<stub_ri_buffer>(params, debug_name);
}

std::unique_ptr<ri_layout_factory> stub_render_interface::create_layout_factory(ri_data_layout layout, ri_layout_usage usage)
{
    return std::make_unique<stub_ri_layout_factory>(layout, usage);
}

std::unique_ptr<ri_query> stub_render_interface::create_query(const ri_query::create_params& params, const char* debug_name)
{
    return std::make_unique<stub_ri_query>(params, debug_name);
}

std::unique_ptr<ri_raytracing_blas> stub_render_interface::create_raytracing_blas(const char* debug_name)
{
    return std::make_unique<stub_ri_raytracing_blas>();
}

std::unique_ptr<ri_raytracing_tlas> stub_render_interface::create_raytracing_tlas(const char* debug_name)
{
    return std::make_unique<stub_ri_raytracing_tlas>();
}

std::unique_ptr<ri_staging_buffer> stub_render_interface::create_staging_buffer(const ri_staging_buffer::create_params& params, std::span<uint8_t> linear_data)
{
    return std::make_unique<stub_ri_staging_buffer>(params, linear_data);
}

ri_command_queue& stub_render_interface::get_graphics_queue()
{
    return m_graphics_queue;
}

ri_command_queue& stub_render_interface::get_copy_queue()
{
    return m_copy_queue;
}

size_t stub_render_interface::get_pipeline_depth()
{
    return k_pipeline_depth;
}

void stub_render_interface::defer_delete(deferred_delete_function_t&& func)
{
    func();
}

void stub_render_interface::get_vram_usage(size_t& out_local, size_t& out_non_local)
{
    out_local = 0;
    out_non_local = 0;
}

void stub_render_interface::get_vram_total(size_t& out_local_total, size_t& out_non_local_total)
{
    out_local_total = 0;
    out_non_local_total = 0;
}

size_t stub_render_interface::get_cube_map_face_index(ri_cube_map_face face)
{
    return static_cast<size_t>(face);
}

bool stub_render_interface::check_feature(ri_feature feature)
{
    return false;
}

}; // namespace ws
