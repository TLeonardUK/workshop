// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

#include "workshop.render_interface/ri_pipeline.h"
#include "workshop.render_interface/ri_param_block_archetype.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_sampler.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_staging_buffer.h"
#include "workshop.render_interface/ri_layout_factory.h"
#include "workshop.render_interface/ri_query.h"

namespace ws {

class window;
class ri_swapchain;
class ri_fence;
class ri_command_queue;
class ri_command_list;
class ri_param_block_archetype;
class ri_shader_compiler;
class ri_texture_compiler;
class ri_buffer;
class ri_layout_factory;
class ri_query;
class ri_raytracing_blas;
class ri_raytracing_tlas;
class ri_staging_buffer;

// ================================================================================================
//  Types of renderer implementations available. Make sure to update if you add new ones.
// ================================================================================================
enum class ri_interface_type
{
#ifdef WS_WINDOWS
    dx12
#endif
};

// ================================================================================================
//  Engine interface for all rendering functionality.
// ================================================================================================
class ri_interface
{
public:
    virtual ~ri_interface() {}

    using deferred_delete_function_t = std::function<void()>;

    // Registers all the steps required to initialize the rendering system.
    // Interacting with this class without successfully running these steps is undefined.
    virtual void register_init(init_list& list) = 0;

    // Informs the renderer that a new frame is starting to be rendered. The 
    // render can use this notification to update per-frame allocations and do
    // any general bookkeeping required.
    virtual void begin_frame() = 0;

    // Informs the renderer that the frame has finished rendering.
    virtual void end_frame() = 0;

    // Data uploads normally occur at the start of a frame. This can be called
    // to flush any pending uploads mid-frame. This is mostly useful for uploading things like
    // param blocks that have been updated this frame and need to be reflected this frame.
    virtual void flush_uploads() = 0;

    // Creates a swapchain for rendering to the given window.
    virtual std::unique_ptr<ri_swapchain> create_swapchain(window& for_window, const char* debug_name = nullptr) = 0;
    
    // Creates a fence for syncronisation between the cpu and gpu.
    virtual std::unique_ptr<ri_fence> create_fence(const char* debug_name = nullptr) = 0;

    // Creates a class to handle compiling shaders for offline use.
    virtual std::unique_ptr<ri_shader_compiler> create_shader_compiler() = 0;

    // Creates a class to handle compiling textures for offline use.
    virtual std::unique_ptr<ri_texture_compiler> create_texture_compiler() = 0;

    // Creates a pipeline describing the gpu state at the point of a draw call.
    virtual std::unique_ptr<ri_pipeline> create_pipeline(const ri_pipeline::create_params& params, const char* debug_name = nullptr) = 0;

    // Creates an archetype that represents a param block type with the given layout and scope.
    virtual std::unique_ptr<ri_param_block_archetype> create_param_block_archetype(const ri_param_block_archetype::create_params& params, const char* debug_name = nullptr) = 0;

    // Creates a texture based on the description given.
    virtual std::unique_ptr<ri_texture> create_texture(const ri_texture::create_params& params, const char* debug_name = nullptr) = 0;

    // Creates a sampler based on the description given.
    virtual std::unique_ptr<ri_sampler> create_sampler(const ri_sampler::create_params& params, const char* debug_name = nullptr) = 0;

    // Creates a buffer of an arbitrary size.
    virtual std::unique_ptr<ri_buffer> create_buffer(const ri_buffer::create_params& params, const char* debug_name = nullptr) = 0;

    // Creates a factory for laying out buffer data in a format consumable by the gpu.
    virtual std::unique_ptr<ri_layout_factory> create_layout_factory(ri_data_layout layout, ri_layout_usage usage) = 0;

    // Creates a factory for laying out buffer data in a format consumable by the gpu.
    virtual std::unique_ptr<ri_query> create_query(const ri_query::create_params& params, const char* debug_name = nullptr) = 0;

    // Creates a bottom level acceleration structure for raytracing.
    virtual std::unique_ptr<ri_raytracing_blas> create_raytracing_blas(const char* debug_name = nullptr) = 0;

    // Creates a top level acceleration structure for raytracing.
    virtual std::unique_ptr<ri_raytracing_tlas> create_raytracing_tlas(const char* debug_name = nullptr) = 0;

    // Creates a new staging buffer to use for upload data to the gpu.
    virtual std::unique_ptr<ri_staging_buffer> create_staging_buffer(const ri_staging_buffer::create_params& params, std::span<uint8_t> linear_data) = 0;
    
    // Gets the main graphics command queue responsible for raster ops.
    virtual ri_command_queue& get_graphics_queue() = 0;

    // Gets the main graphics command queue responsible for performing memory copies.
    virtual ri_command_queue& get_copy_queue() = 0;

    // Gets the maximum number of frames that can be in flight at the same time.
    virtual size_t get_pipeline_depth() = 0;

    // Used to defer a resource deletion until the gpu is no longer referencing it.
    virtual void defer_delete(const deferred_delete_function_t& func) = 0;

    // Gets the number of bytes of vram currently in use by the application.
    // Local memory is dedicated vram, non-local is system shared memory.
    virtual void get_vram_usage(size_t& out_local, size_t& out_non_local) = 0;

    // Gets the number of bytes of vram currently available to the application.
    // Local memory is dedicated vram, non-local is system shared memory.
    virtual void get_vram_total(size_t& out_local_total, size_t& out_non_local_total) = 0;

    // Gets the texture slice that represents a given cube map face.
    virtual size_t get_cube_map_face_index(ri_cube_map_face face) = 0;
    
    // Checks if a given feature is supported by the device.
    virtual bool check_feature(ri_feature feature) = 0;

};

}; // namespace ws
