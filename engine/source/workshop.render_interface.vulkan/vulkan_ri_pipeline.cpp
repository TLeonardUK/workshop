// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_pipeline.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_table.h"
#include "workshop.render_interface.vulkan/vulkan_ri_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_types.h"
#include "workshop.render_interface/ri_param_block_archetype.h"
#include "workshop.core/math/math.h"

#include <array>
#include <cstring>
#include <vector>

namespace ws {

namespace {

VkShaderStageFlagBits ri_to_vulkan_shader_stage(ri_shader_stage stage)
{
    static std::array<VkShaderStageFlagBits, static_cast<int>(ri_shader_stage::COUNT)> conversion = {
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
        VK_SHADER_STAGE_GEOMETRY_BIT,
        VK_SHADER_STAGE_COMPUTE_BIT,

        VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        VK_SHADER_STAGE_MISS_BIT_KHR,
        VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
    };

    return conversion[static_cast<int>(stage)];
}

}; // namespace

vulkan_ri_pipeline::vulkan_ri_pipeline(vulkan_render_interface& renderer, const create_params& params, const char* debug_name)
    : m_renderer(renderer)
    , m_params(params)
    , m_debug_name(debug_name ? debug_name : "")
{
    m_is_compute = !params.stages[(int)ri_shader_stage::compute].file.empty();
    m_is_raytracing = !params.ray_hitgroups.empty() || !params.ray_missgroups.empty();
}

vulkan_ri_pipeline::~vulkan_ri_pipeline()
{
    if (m_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_renderer.get_device(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipeline_layout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_renderer.get_device(), m_pipeline_layout, nullptr);
        m_pipeline_layout = VK_NULL_HANDLE;
    }
}

result<void> vulkan_ri_pipeline::create_resources()
{
    // Work out which archetypes need a push-constant-bound gpu address (global/draw scope)
    // vs which are read bindlessly by the shader itself (instance/indirect scope).
    size_t next_push_constant_index = 0;
    for (ri_param_block_archetype* archetype : m_params.param_block_archetypes)
    {
        ri_data_scope scope = archetype->get_create_params().scope;
        if (scope == ri_data_scope::global || scope == ri_data_scope::draw)
        {
            m_push_constant_offsets[archetype] = next_push_constant_index * sizeof(VkDeviceAddress);
            next_push_constant_index++;
        }
    }

    // Set index 0 must exist (vulkan requires dense set indices from 0), but is unused -
    // the 9 real bindless sets are fixed at indices 1-9 by the HLSL register space layout.
    // Uses the renderer's single persistent dummy layout rather than creating one per-pipeline -
    // a VkPipelineLayout's constituent set layouts must stay alive for as long as anything
    // created from it is still used (eg. vkCmdBindPipeline), not just through creation, so a
    // per-pipeline layout destroyed after create_resources() would go dangling the moment this
    // pipeline was actually bound in a later frame.
    std::vector<VkDescriptorSetLayout> set_layouts;
    set_layouts.reserve(1 + vulkan_render_interface::k_bindless_set_count);
    set_layouts.push_back(m_renderer.get_dummy_set_layout());
    for (size_t i = 0; i < vulkan_render_interface::k_bindless_set_count; i++)
    {
        set_layouts.push_back(m_renderer.get_descriptor_table(static_cast<ri_descriptor_table>(i)).get_descriptor_set_layout());
    }

    VkPushConstantRange push_constant_range = {};
    push_constant_range.stageFlags = VK_SHADER_STAGE_ALL;
    push_constant_range.offset = 0;
    push_constant_range.size = static_cast<uint32_t>(next_push_constant_index * sizeof(VkDeviceAddress));

    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount = static_cast<uint32_t>(set_layouts.size());
    layout_create_info.pSetLayouts = set_layouts.data();
    if (push_constant_range.size > 0)
    {
        layout_create_info.pushConstantRangeCount = 1;
        layout_create_info.pPushConstantRanges = &push_constant_range;
    }

    VkResult vk_result = vkCreatePipelineLayout(m_renderer.get_device(), &layout_create_info, nullptr, &m_pipeline_layout);

    if (!m_renderer.check_result(vk_result, "vkCreatePipelineLayout"))
    {
        return standard_errors::failed;
    }

    bool ok;
    if (m_is_raytracing)
    {
        ok = create_raytracing_pipeline();
    }
    else if (m_is_compute)
    {
        ok = create_compute_pipeline();
    }
    else
    {
        ok = create_graphics_pipeline();
    }

    // Bytecode is only needed up to pipeline creation, drop it afterwards.
    for (create_params::stage& s : m_params.stages)
    {
        s.bytecode.clear();
        s.bytecode.shrink_to_fit();
    }
    for (create_params::ray_hitgroup& group : m_params.ray_hitgroups)
    {
        for (create_params::stage& s : group.stages)
        {
            s.bytecode.clear();
            s.bytecode.shrink_to_fit();
        }
    }
    for (create_params::ray_missgroup& group : m_params.ray_missgroups)
    {
        group.ray_miss_stage.bytecode.clear();
        group.ray_miss_stage.bytecode.shrink_to_fit();
    }

    if (!ok)
    {
        return standard_errors::failed;
    }

    return true;
}

bool vulkan_ri_pipeline::create_graphics_pipeline()
{
    std::vector<VkPipelineShaderStageCreateInfo> stage_infos;
    std::vector<VkShaderModule> shader_modules;

    auto add_stage = [&](ri_shader_stage stage_type)
    {
        create_params::stage& stage_params = m_params.stages[static_cast<int>(stage_type)];
        if (stage_params.bytecode.empty())
        {
            return;
        }

        VkShaderModuleCreateInfo module_create_info = {};
        module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_create_info.codeSize = stage_params.bytecode.size();
        module_create_info.pCode = reinterpret_cast<const uint32_t*>(stage_params.bytecode.data());

        VkShaderModule module = VK_NULL_HANDLE;
        VkResult vk_result = vkCreateShaderModule(m_renderer.get_device(), &module_create_info, nullptr, &module);
        if (!m_renderer.check_result(vk_result, "vkCreateShaderModule"))
        {
            return;
        }

        shader_modules.push_back(module);

        VkPipelineShaderStageCreateInfo stage_info = {};
        stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage_info.stage = ri_to_vulkan_shader_stage(stage_type);
        stage_info.module = module;
        stage_info.pName = stage_params.entry_point.c_str();

        stage_infos.push_back(stage_info);
    };

    add_stage(ri_shader_stage::vertex);
    add_stage(ri_shader_stage::pixel);
    add_stage(ri_shader_stage::domain);
    add_stage(ri_shader_stage::hull);
    add_stage(ri_shader_stage::geometry);

    // Bindless vertex pulling, no fixed-function vertex input state needed.
    VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.topology = ri_to_vulkan(m_params.render_state.topology);

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_state = {};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.polygonMode = ri_to_vulkan(m_params.render_state.fill_mode);
    rasterization_state.cullMode = ri_to_vulkan(m_params.render_state.cull_mode);
    rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state.depthClampEnable = !m_params.render_state.depth_clip_enabled;
    rasterization_state.depthBiasEnable = (m_params.render_state.depth_bias != 0 || m_params.render_state.slope_scaled_depth_bias != 0.0f);
    rasterization_state.depthBiasConstantFactor = static_cast<float>(m_params.render_state.depth_bias);
    rasterization_state.depthBiasClamp = m_params.render_state.depth_bias_clamp;
    rasterization_state.depthBiasSlopeFactor = m_params.render_state.slope_scaled_depth_bias;
    rasterization_state.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state = {};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.rasterizationSamples = static_cast<VkSampleCountFlagBits>(m_params.render_state.multisample_enabled ? m_params.render_state.multisample_count : 1);
    multisample_state.alphaToCoverageEnable = m_params.render_state.alpha_to_coverage;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.depthTestEnable = m_params.render_state.depth_test_enabled;
    depth_stencil_state.depthWriteEnable = m_params.render_state.depth_write_enabled;
    depth_stencil_state.depthCompareOp = ri_to_vulkan(m_params.render_state.depth_compare_op);
    depth_stencil_state.stencilTestEnable = m_params.render_state.stencil_test_enabled;
    depth_stencil_state.front.failOp = ri_to_vulkan(m_params.render_state.stencil_front_face_fail_op);
    depth_stencil_state.front.depthFailOp = ri_to_vulkan(m_params.render_state.stencil_front_face_depth_fail_op);
    depth_stencil_state.front.passOp = ri_to_vulkan(m_params.render_state.stencil_front_face_pass_op);
    depth_stencil_state.front.compareOp = ri_to_vulkan(m_params.render_state.stencil_front_face_compare_op);
    depth_stencil_state.front.compareMask = m_params.render_state.stencil_read_mask;
    depth_stencil_state.front.writeMask = m_params.render_state.stencil_write_mask;
    depth_stencil_state.back.failOp = ri_to_vulkan(m_params.render_state.stencil_back_face_fail_op);
    depth_stencil_state.back.depthFailOp = ri_to_vulkan(m_params.render_state.stencil_back_face_depth_fail_op);
    depth_stencil_state.back.passOp = ri_to_vulkan(m_params.render_state.stencil_back_face_pass_op);
    depth_stencil_state.back.compareOp = ri_to_vulkan(m_params.render_state.stencil_back_face_compare_op);
    depth_stencil_state.back.compareMask = m_params.render_state.stencil_read_mask;
    depth_stencil_state.back.writeMask = m_params.render_state.stencil_write_mask;

    std::vector<VkPipelineColorBlendAttachmentState> blend_attachments;
    for (size_t i = 0; i < m_params.color_formats.size(); i++)
    {
        VkPipelineColorBlendAttachmentState attachment = {};
        attachment.blendEnable = m_params.render_state.blend_enabled[i];
        attachment.colorBlendOp = ri_to_vulkan(m_params.render_state.blend_op[i]);
        attachment.srcColorBlendFactor = ri_to_vulkan(m_params.render_state.blend_source_op[i]);
        attachment.dstColorBlendFactor = ri_to_vulkan(m_params.render_state.blend_destination_op[i]);
        attachment.alphaBlendOp = ri_to_vulkan(m_params.render_state.blend_alpha_op[i]);
        attachment.srcAlphaBlendFactor = ri_to_vulkan(m_params.render_state.blend_alpha_source_op[i]);
        attachment.dstAlphaBlendFactor = ri_to_vulkan(m_params.render_state.blend_alpha_destination_op[i]);
        attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blend_attachments.push_back(attachment);
    }

    VkPipelineColorBlendStateCreateInfo color_blend_state = {};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.attachmentCount = static_cast<uint32_t>(blend_attachments.size());
    color_blend_state.pAttachments = blend_attachments.data();

    std::array<VkDynamicState, 6> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    std::vector<VkFormat> color_attachment_formats;
    for (ri_texture_format format : m_params.color_formats)
    {
        color_attachment_formats.push_back(ri_to_vulkan(format));
    }

    VkPipelineRenderingCreateInfo rendering_create_info = {};
    rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_create_info.colorAttachmentCount = static_cast<uint32_t>(color_attachment_formats.size());
    rendering_create_info.pColorAttachmentFormats = color_attachment_formats.data();
    if (m_params.depth_format != ri_texture_format::Undefined)
    {
        rendering_create_info.depthAttachmentFormat = ri_to_vulkan(m_params.depth_format);
        if (m_params.depth_format == ri_texture_format::D24_UNORM_S8_UINT)
        {
            rendering_create_info.stencilAttachmentFormat = rendering_create_info.depthAttachmentFormat;
        }
    }

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext = &rendering_create_info;
    pipeline_create_info.stageCount = static_cast<uint32_t>(stage_infos.size());
    pipeline_create_info.pStages = stage_infos.data();
    pipeline_create_info.pVertexInputState = &vertex_input_state;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state;
    pipeline_create_info.pViewportState = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterization_state;
    pipeline_create_info.pMultisampleState = &multisample_state;
    pipeline_create_info.pDepthStencilState = &depth_stencil_state;
    pipeline_create_info.pColorBlendState = &color_blend_state;
    pipeline_create_info.pDynamicState = &dynamic_state;
    pipeline_create_info.layout = m_pipeline_layout;

    VkResult vk_result = vkCreateGraphicsPipelines(m_renderer.get_device(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &m_pipeline);

    for (VkShaderModule module : shader_modules)
    {
        vkDestroyShaderModule(m_renderer.get_device(), module, nullptr);
    }

    if (!m_renderer.check_result(vk_result, "vkCreateGraphicsPipelines"))
    {
        return false;
    }

    return true;
}

bool vulkan_ri_pipeline::create_compute_pipeline()
{
    create_params::stage& stage_params = m_params.stages[static_cast<int>(ri_shader_stage::compute)];
    if (stage_params.bytecode.empty())
    {
        db_error(render_interface, "Compute pipeline '%s' has no compute shader bytecode.", m_debug_name.c_str());
        return false;
    }

    VkShaderModuleCreateInfo module_create_info = {};
    module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_create_info.codeSize = stage_params.bytecode.size();
    module_create_info.pCode = reinterpret_cast<const uint32_t*>(stage_params.bytecode.data());

    VkShaderModule module = VK_NULL_HANDLE;
    VkResult vk_result = vkCreateShaderModule(m_renderer.get_device(), &module_create_info, nullptr, &module);
    if (!m_renderer.check_result(vk_result, "vkCreateShaderModule"))
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo stage_info = {};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_info.module = module;
    stage_info.pName = stage_params.entry_point.c_str();

    VkComputePipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_create_info.stage = stage_info;
    pipeline_create_info.layout = m_pipeline_layout;

    vk_result = vkCreateComputePipelines(m_renderer.get_device(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &m_pipeline);

    vkDestroyShaderModule(m_renderer.get_device(), module, nullptr);

    if (!m_renderer.check_result(vk_result, "vkCreateComputePipelines"))
    {
        return false;
    }

    return true;
}

bool vulkan_ri_pipeline::create_raytracing_pipeline()
{
    std::vector<VkPipelineShaderStageCreateInfo> stage_infos;
    std::vector<VkShaderModule> shader_modules;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;

    // Index of the group that will hold each shader once built, keyed the same way build_sbt()
    // looks them up (nullopt/empty means no group was defined for that slot).
    size_t raygen_group_index = SIZE_MAX;
    std::unordered_map<size_t, size_t> miss_group_index_by_type;
    std::unordered_map<size_t, size_t> hit_group_index_by_domain_type;

    auto add_stage = [&](const create_params::stage& stage_params) -> uint32_t
    {
        if (stage_params.bytecode.empty())
        {
            return VK_SHADER_UNUSED_KHR;
        }

        VkShaderModuleCreateInfo module_create_info = {};
        module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_create_info.codeSize = stage_params.bytecode.size();
        module_create_info.pCode = reinterpret_cast<const uint32_t*>(stage_params.bytecode.data());

        VkShaderModule module = VK_NULL_HANDLE;
        VkResult vk_result = vkCreateShaderModule(m_renderer.get_device(), &module_create_info, nullptr, &module);
        if (!m_renderer.check_result(vk_result, "vkCreateShaderModule"))
        {
            return VK_SHADER_UNUSED_KHR;
        }

        shader_modules.push_back(module);

        VkPipelineShaderStageCreateInfo stage_info = {};
        stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage_info.module = module;
        stage_info.pName = stage_params.entry_point.c_str();

        // Ray hit/miss/generation stages all share bytecode compiled as lib_6_x - the stage
        // flag on the pipeline shader stage still needs to reflect the specific role, which
        // the caller doesn't have handy here, so infer it from which stage array this came
        // from isn't possible generically - callers pass it in via stage_info.stage below.
        stage_infos.push_back(stage_info);
        return static_cast<uint32_t>(stage_infos.size() - 1);
    };

    // Ray generation - a single mandatory general group.
    {
        const create_params::stage& stage_params = m_params.stages[static_cast<int>(ri_shader_stage::ray_generation)];
        uint32_t index = add_stage(stage_params);
        if (index != VK_SHADER_UNUSED_KHR)
        {
            stage_infos[index].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

            VkRayTracingShaderGroupCreateInfoKHR group = {};
            group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            group.generalShader = index;
            group.closestHitShader = VK_SHADER_UNUSED_KHR;
            group.anyHitShader = VK_SHADER_UNUSED_KHR;
            group.intersectionShader = VK_SHADER_UNUSED_KHR;

            raygen_group_index = groups.size();
            groups.push_back(group);
        }
    }

    // Miss groups.
    for (create_params::ray_missgroup& missgroup : m_params.ray_missgroups)
    {
        uint32_t index = add_stage(missgroup.ray_miss_stage);
        if (index == VK_SHADER_UNUSED_KHR)
        {
            continue;
        }

        stage_infos[index].stage = VK_SHADER_STAGE_MISS_BIT_KHR;

        VkRayTracingShaderGroupCreateInfoKHR group = {};
        group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        group.generalShader = index;
        group.closestHitShader = VK_SHADER_UNUSED_KHR;
        group.anyHitShader = VK_SHADER_UNUSED_KHR;
        group.intersectionShader = VK_SHADER_UNUSED_KHR;

        miss_group_index_by_type[missgroup.type] = groups.size();
        groups.push_back(group);
    }

    // Hit groups.
    for (create_params::ray_hitgroup& hitgroup : m_params.ray_hitgroups)
    {
        const create_params::stage& any_hit_stage = hitgroup.stages[static_cast<int>(ri_shader_stage::ray_any_hit)];
        const create_params::stage& closest_hit_stage = hitgroup.stages[static_cast<int>(ri_shader_stage::ray_closest_hit)];
        const create_params::stage& intersection_stage = hitgroup.stages[static_cast<int>(ri_shader_stage::ray_intersection)];

        uint32_t any_hit_index = add_stage(any_hit_stage);
        if (any_hit_index != VK_SHADER_UNUSED_KHR)
        {
            stage_infos[any_hit_index].stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        }

        uint32_t closest_hit_index = add_stage(closest_hit_stage);
        if (closest_hit_index != VK_SHADER_UNUSED_KHR)
        {
            stage_infos[closest_hit_index].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        }

        uint32_t intersection_index = add_stage(intersection_stage);

        VkRayTracingShaderGroupCreateInfoKHR group = {};
        group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        group.generalShader = VK_SHADER_UNUSED_KHR;
        group.anyHitShader = any_hit_index;
        group.closestHitShader = closest_hit_index;

        if (intersection_index != VK_SHADER_UNUSED_KHR)
        {
            stage_infos[intersection_index].stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
            group.intersectionShader = intersection_index;
        }
        else
        {
            group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            group.intersectionShader = VK_SHADER_UNUSED_KHR;
        }

        hit_group_index_by_domain_type[hitgroup.domain * m_renderer.get_ray_type_count() + hitgroup.type] = groups.size();
        groups.push_back(group);
    }

    // Note: pLibraryInfo/pLibraryInterface are deliberately left null - they're only valid
    // when VK_KHR_pipeline_library is enabled, which this backend doesn't need since it always
    // builds one complete, standalone pipeline rather than linking pipeline libraries together.
    VkRayTracingPipelineCreateInfoKHR pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipeline_create_info.stageCount = static_cast<uint32_t>(stage_infos.size());
    pipeline_create_info.pStages = stage_infos.data();
    pipeline_create_info.groupCount = static_cast<uint32_t>(groups.size());
    pipeline_create_info.pGroups = groups.data();
    pipeline_create_info.maxPipelineRayRecursionDepth = m_params.render_state.max_rt_recursion;
    pipeline_create_info.layout = m_pipeline_layout;

    VkResult vk_result = m_renderer.vkCreateRayTracingPipelinesKHR_fn(m_renderer.get_device(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &m_pipeline);

    for (VkShaderModule module : shader_modules)
    {
        vkDestroyShaderModule(m_renderer.get_device(), module, nullptr);
    }

    if (!m_renderer.check_result(vk_result, "vkCreateRayTracingPipelinesKHR"))
    {
        return false;
    }

    return build_sbt(groups.size(), raygen_group_index, miss_group_index_by_type, hit_group_index_by_domain_type);
}

bool vulkan_ri_pipeline::build_sbt(
    size_t group_count,
    size_t raygen_group_index,
    const std::unordered_map<size_t, size_t>& miss_group_index_by_type,
    const std::unordered_map<size_t, size_t>& hit_group_index_by_domain_type)
{
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rt_props = m_renderer.get_raytracing_pipeline_properties();

    size_t handle_size = rt_props.shaderGroupHandleSize;
    size_t handle_size_aligned = math::round_up_multiple(handle_size, static_cast<size_t>(rt_props.shaderGroupHandleAlignment));
    size_t base_alignment = rt_props.shaderGroupBaseAlignment;

    m_sbt_record_stride = handle_size_aligned;

    std::vector<uint8_t> all_handles(group_count * handle_size);
    VkResult vk_result = m_renderer.vkGetRayTracingShaderGroupHandlesKHR_fn(m_renderer.get_device(), m_pipeline, 0, static_cast<uint32_t>(group_count), all_handles.size(), all_handles.data());
    if (!m_renderer.check_result(vk_result, "vkGetRayTracingShaderGroupHandlesKHR"))
    {
        return false;
    }

    auto copy_handle = [&](std::vector<uint8_t>& sbt_data, size_t group_index)
    {
        size_t dest_offset = sbt_data.size();
        sbt_data.resize(sbt_data.size() + handle_size_aligned);
        memcpy(sbt_data.data() + dest_offset, all_handles.data() + group_index * handle_size, handle_size);
    };

    auto align = [](std::vector<uint8_t>& sbt_data, size_t alignment)
    {
        size_t remainder = sbt_data.size() % alignment;
        if (remainder != 0)
        {
            sbt_data.resize(sbt_data.size() + (alignment - remainder));
        }
    };

    std::vector<uint8_t> sbt_data;

    // Ray generation record - always exactly one record, no padding entries.
    m_ray_generation_shader_offset = 0;
    if (raygen_group_index != SIZE_MAX)
    {
        copy_handle(sbt_data, raygen_group_index);
    }
    else
    {
        sbt_data.resize(sbt_data.size() + handle_size_aligned);
    }

    // Miss shader table, indexed by ray type.
    align(sbt_data, base_alignment);
    m_ray_miss_table_offset = sbt_data.size();

    size_t ray_type_count = m_renderer.get_ray_type_count();
    size_t ray_domain_count = m_renderer.get_ray_domain_count();

    for (size_t type = 0; type < ray_type_count; type++)
    {
        auto iter = miss_group_index_by_type.find(type);
        if (iter != miss_group_index_by_type.end())
        {
            copy_handle(sbt_data, iter->second);
        }
        else
        {
            sbt_data.resize(sbt_data.size() + handle_size_aligned);
        }
    }

    // Hit group table, indexed by [domain][type].
    align(sbt_data, base_alignment);
    m_ray_hit_group_table_offset = sbt_data.size();

    for (size_t domain = 0; domain < ray_domain_count; domain++)
    {
        for (size_t type = 0; type < ray_type_count; type++)
        {
            auto iter = hit_group_index_by_domain_type.find(domain * ray_type_count + type);
            if (iter != hit_group_index_by_domain_type.end())
            {
                copy_handle(sbt_data, iter->second);
            }
            else
            {
                sbt_data.resize(sbt_data.size() + handle_size_aligned);
            }
        }
    }

    ri_buffer::create_params sbt_params;
    sbt_params.element_count = 1;
    sbt_params.element_size = sbt_data.size();
    sbt_params.usage = ri_buffer_usage::raytracing_shader_binding_table;
    sbt_params.linear_data = std::span<uint8_t>(sbt_data.begin(), sbt_data.end());

    m_shader_binding_table = m_renderer.create_buffer(sbt_params, string_format("%s : shader binding table", m_debug_name.c_str()).c_str());
    if (!m_shader_binding_table)
    {
        return false;
    }

    m_is_raytracing = true;

    return true;
}

bool vulkan_ri_pipeline::is_compute() const
{
    return m_is_compute;
}

bool vulkan_ri_pipeline::is_raytracing() const
{
    return m_is_raytracing;
}

VkStridedDeviceAddressRegionKHR vulkan_ri_pipeline::get_ray_generation_shader_record() const
{
    VkDeviceAddress base = static_cast<vulkan_ri_buffer*>(m_shader_binding_table.get())->get_gpu_address();

    VkStridedDeviceAddressRegionKHR region = {};
    region.deviceAddress = base + m_ray_generation_shader_offset;
    region.stride = m_sbt_record_stride;
    region.size = m_sbt_record_stride;
    return region;
}

VkStridedDeviceAddressRegionKHR vulkan_ri_pipeline::get_miss_shader_table() const
{
    VkDeviceAddress base = static_cast<vulkan_ri_buffer*>(m_shader_binding_table.get())->get_gpu_address();

    VkStridedDeviceAddressRegionKHR region = {};
    region.deviceAddress = base + m_ray_miss_table_offset;
    region.stride = m_sbt_record_stride;
    region.size = m_sbt_record_stride * m_renderer.get_ray_type_count();
    return region;
}

VkStridedDeviceAddressRegionKHR vulkan_ri_pipeline::get_hit_group_table() const
{
    VkDeviceAddress base = static_cast<vulkan_ri_buffer*>(m_shader_binding_table.get())->get_gpu_address();

    VkStridedDeviceAddressRegionKHR region = {};
    region.deviceAddress = base + m_ray_hit_group_table_offset;
    region.stride = m_sbt_record_stride;
    region.size = m_sbt_record_stride * m_renderer.get_ray_type_count() * m_renderer.get_ray_domain_count();
    return region;
}

VkStridedDeviceAddressRegionKHR vulkan_ri_pipeline::get_callable_shader_table() const
{
    VkStridedDeviceAddressRegionKHR region = {};
    return region;
}

const ri_pipeline::create_params& vulkan_ri_pipeline::get_create_params()
{
    return m_params;
}

VkPipeline vulkan_ri_pipeline::get_pipeline()
{
    return m_pipeline;
}

VkPipelineLayout vulkan_ri_pipeline::get_pipeline_layout()
{
    return m_pipeline_layout;
}

VkPipelineBindPoint vulkan_ri_pipeline::get_bind_point()
{
    if (m_is_raytracing)
    {
        return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
    }
    if (m_is_compute)
    {
        return VK_PIPELINE_BIND_POINT_COMPUTE;
    }
    return VK_PIPELINE_BIND_POINT_GRAPHICS;
}

bool vulkan_ri_pipeline::get_push_constant_offset(ri_param_block_archetype* archetype, size_t& out_offset)
{
    auto iter = m_push_constant_offsets.find(archetype);
    if (iter == m_push_constant_offsets.end())
    {
        return false;
    }

    out_offset = iter->second;
    return true;
}

}; // namespace ws
