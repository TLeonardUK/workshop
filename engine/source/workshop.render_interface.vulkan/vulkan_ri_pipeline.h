// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_pipeline.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.core/utils/result.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace ws {

class vulkan_render_interface;

// ================================================================================================
//  Implementation of a pipeline using Vulkan.
// ================================================================================================
class vulkan_ri_pipeline : public ri_pipeline
{
public:
    vulkan_ri_pipeline(vulkan_render_interface& renderer, const create_params& params, const char* debug_name);
    virtual ~vulkan_ri_pipeline();

    // Creates the vulkan resources required by this pipeline.
    result<void> create_resources();

    virtual const create_params& get_create_params() override;

    VkPipeline get_pipeline();
    VkPipelineLayout get_pipeline_layout();
    VkPipelineBindPoint get_bind_point();

    bool is_compute() const;
    bool is_raytracing() const;

    // Index into create_params::param_block_archetypes -> push constant offset, for
    // global/draw scope archetypes only (instance/indirect scope archetypes are read
    // bindlessly by the shader itself and never bound directly).
    bool get_push_constant_offset(ri_param_block_archetype* archetype, size_t& out_offset);

    // Shader binding table regions, only valid when is_raytracing() is true.
    VkStridedDeviceAddressRegionKHR get_ray_generation_shader_record() const;
    VkStridedDeviceAddressRegionKHR get_miss_shader_table() const;
    VkStridedDeviceAddressRegionKHR get_hit_group_table() const;
    VkStridedDeviceAddressRegionKHR get_callable_shader_table() const;

private:
    bool create_graphics_pipeline();
    bool create_compute_pipeline();
    bool create_raytracing_pipeline();
    bool build_sbt(
        size_t group_count,
        size_t raygen_group_index,
        const std::unordered_map<size_t, size_t>& miss_group_index_by_type,
        const std::unordered_map<size_t, size_t>& hit_group_index_by_domain_type);

private:
    vulkan_render_interface& m_renderer;
    create_params m_params;
    std::string m_debug_name;

    bool m_is_compute = false;
    bool m_is_raytracing = false;

    VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    std::unordered_map<ri_param_block_archetype*, size_t> m_push_constant_offsets;

    // Shader binding table layout - see build_sbt().
    size_t m_sbt_record_stride = 0;
    size_t m_ray_generation_shader_offset = 0;
    size_t m_ray_miss_table_offset = 0;
    size_t m_ray_hit_group_table_offset = 0;
    std::unique_ptr<ri_buffer> m_shader_binding_table;

};

}; // namespace ws
