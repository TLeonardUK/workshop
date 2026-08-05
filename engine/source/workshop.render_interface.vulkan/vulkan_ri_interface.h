// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"

namespace ws {

// ================================================================================================
//  Implementation of a renderer using Vulkan.
// ================================================================================================
class vulkan_render_interface : public ri_interface
{
public:
    vulkan_render_interface(size_t ray_type_count, size_t ray_domain_count);
    virtual ~vulkan_render_interface();

    virtual void register_init(init_list& list) override;

    virtual void begin_frame() override;
    virtual void end_frame() override;

    virtual void flush_uploads() override;

    virtual std::unique_ptr<ri_swapchain> create_swapchain(window& for_window, const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_fence> create_fence(const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_shader_compiler> create_shader_compiler() override;
    virtual std::unique_ptr<ri_texture_compiler> create_texture_compiler() override;
    virtual std::unique_ptr<ri_pipeline> create_pipeline(const ri_pipeline::create_params& params, const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_param_block_archetype> create_param_block_archetype(const ri_param_block_archetype::create_params& params, const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_texture> create_texture(const ri_texture::create_params& params, const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_sampler> create_sampler(const ri_sampler::create_params& params, const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_buffer> create_buffer(const ri_buffer::create_params& params, const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_layout_factory> create_layout_factory(ri_data_layout layout, ri_layout_usage usage) override;
    virtual std::unique_ptr<ri_query> create_query(const ri_query::create_params& params, const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_raytracing_blas> create_raytracing_blas(const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_raytracing_tlas> create_raytracing_tlas(const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_staging_buffer> create_staging_buffer(const ri_staging_buffer::create_params& params, std::span<uint8_t> linear_data) override;

    virtual ri_command_queue& get_graphics_queue() override;
    virtual ri_command_queue& get_copy_queue() override;

    virtual size_t get_pipeline_depth() override;

    virtual void defer_delete(deferred_delete_function_t&& func) override;

    virtual void get_vram_usage(size_t& out_local, size_t& out_non_local) override;
    virtual void get_vram_total(size_t& out_local_total, size_t& out_non_local_total) override;

    virtual size_t get_cube_map_face_index(ri_cube_map_face face) override;

    virtual bool check_feature(ri_feature feature) override;

private:
    constexpr static size_t k_pipeline_depth = 3;

    size_t m_ray_type_count;
    size_t m_ray_domain_count;

    vulkan_ri_command_queue m_graphics_queue;
    vulkan_ri_command_queue m_copy_queue;

};

}; // namespace ws
