// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.render_interface/ri_types.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_table.h"
#include "workshop.render_interface.vulkan/vulkan_ri_small_buffer_allocator.h"
#include "workshop.render_interface.vulkan/vulkan_ri_upload_manager.h"
#include "workshop.core/utils/result.h"

#include <array>
#include <memory>
#include <unordered_set>

namespace ws {

class vulkan_ri_param_block;
class vulkan_ri_upload_manager;
class vulkan_ri_tile_manager;
class vulkan_ri_query_manager;

// ================================================================================================
//  Implementation of a renderer using Vulkan.
// ================================================================================================
class vulkan_render_interface : public ri_interface
{
public:
    // How many frames can be in the pipeline at a given time, including
    // the one currently being built. 
    // The number of swap chain targets is one lower than this.
    constexpr static size_t k_max_pipeline_depth = 3;

    // Maximum amount of descriptors in each table.
    constexpr static std::array<size_t, static_cast<int>(ri_descriptor_table::COUNT)> k_descriptor_table_sizes = {
        100,    // texture_1d
        100000, // texture_2d
        1000,   // texture_3d
        100,    // texture_cube
        100,    // sampler
        200000, // buffer
        200000, // rwbuffer
        200000, // rwbuffer_shader_invisible
        1000,   // rwtexture_2d
        100000, // tlas
        1000,   // render_target
        1000,   // depth_stencil
    };

    // Maximum amount of queries that can be allocated.
    constexpr static size_t k_maximum_queries = 200;

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

    // Marks a param block as dirty and tells the param block to upload its state
    // the next time uploads are flushed.
    void queue_dirty_param_block(vulkan_ri_param_block* block);
    void dequeue_dirty_param_block(vulkan_ri_param_block* block);
    std::recursive_mutex& get_dirty_param_block_mutex();

    // Drains all of the defered deletes without regard for which frame they should
    // be destroyed on. Be -very- careful with this, the only real usecase is when we are
    // draining the entire pipeline at once.
    void drain_deferred();

    // Checks the the vk result for success and returns true if it was succesful. On failure, a debug log
    // is written and any needed debugging information is dumped out, the function then returns false.
    bool check_result(VkResult result, const char* context);

    // Same as check_result but terminates the program on failure.
    void assert_result(VkResult result, const char* context);

    VkDevice get_device();
    VkInstance get_instance();

    result<uint32_t> find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);

    vulkan_ri_descriptor_table& get_descriptor_table(ri_descriptor_table table);
    vulkan_ri_small_buffer_allocator& get_small_buffer_allocator();
    vulkan_ri_upload_manager& get_upload_manager();

private:
    result<void> create_device();
    result<void> destroy_device();

    result<void> create_command_queues();
    result<void> destroy_command_queues();

    result<void> create_heaps();
    result<void> destroy_heaps();

    result<void> create_misc();
    result<void> destroy_misc();

    result<void> check_feature_support();

    bool check_extension_support(const char* name);
    bool check_physical_device_extension_support(const char* name);
    bool check_layer_support(const char* name);

    result<void> get_extensions();
    result<void> get_required_extensions();

    result<void> get_layers();
    result<void> get_required_layers();

    result<void> select_physical_device();
    result<void> select_queue_families();
    result<void> create_logical_device();

    result<void> resolve_instance_functions();
    result<void> resolve_device_functions();

    void process_pending_deletes();

public:
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;

    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;
    PFN_vkCmdCopyAccelerationStructureKHR vkCmdCopyAccelerationStructureKHR = nullptr;
    PFN_vkCmdWriteAccelerationStructuresPropertiesKHR vkCmdWriteAccelerationStructuresPropertiesKHR = nullptr;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = nullptr;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = nullptr;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR = nullptr;
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;

private:
    constexpr static size_t k_pipeline_depth = 3;

    bool m_debug_device = false;
    VkDevice m_device;
    VkPhysicalDevice m_physical_device;
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debug_messenger;

    size_t m_ray_type_count;
    size_t m_ray_domain_count;

    std::unique_ptr<vulkan_ri_command_queue> m_graphics_queue;
    std::unique_ptr<vulkan_ri_command_queue> m_copy_queue;
    std::unique_ptr<vulkan_ri_upload_manager> m_upload_manager = nullptr;
    std::unique_ptr<vulkan_ri_tile_manager> m_tile_manager = nullptr;
    std::unique_ptr<vulkan_ri_query_manager> m_query_manager = nullptr;
    std::unique_ptr<vulkan_ri_small_buffer_allocator> m_small_buffer_allocator = nullptr;

    std::array<std::unique_ptr<vulkan_ri_descriptor_table>, static_cast<int>(ri_descriptor_table::COUNT)> m_descriptor_tables;

    std::array<bool, (int)ri_feature::COUNT> m_feature_support;

    size_t m_frame_index = 0;

    std::mutex m_pending_deletion_mutex;
    std::array<std::vector<deferred_delete_function_t>, k_max_pipeline_depth> m_pending_deletions;

    std::recursive_mutex m_dirty_param_block_mutex;
    std::unordered_set<vulkan_ri_param_block*> m_dirty_param_blocks;
    bool m_flush_upload_reentry = false;

    std::vector<const char*> m_required_physical_device_extensions;
    std::vector<const char*> m_required_extensions;
    std::vector<const char*> m_required_layers;
    std::vector<VkExtensionProperties> m_available_extensions;
    std::vector<VkExtensionProperties> m_available_physical_device_extensions;
    std::vector<VkLayerProperties> m_available_layers;

    int m_graphics_queue_family;
    int m_copy_queue_family;

    size_t m_vram_total_local = 0;
    size_t m_vram_total_non_local = 0;

};

}; // namespace ws
