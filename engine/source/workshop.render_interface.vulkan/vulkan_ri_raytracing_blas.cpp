// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_raytracing_blas.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_list.h"
#include "workshop.render_interface.vulkan/vulkan_ri_buffer.h"
#include "workshop.core/math/math.h"
#include "workshop.core/containers/string.h"

namespace ws {

namespace {

void acceleration_structure_barrier(VkCommandBuffer command_buffer)
{
    VkMemoryBarrier2 memory_barrier = {};
    memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    // Without VK_KHR_ray_tracing_maintenance1 (not enabled on this device),
    // ACCELERATION_STRUCTURE_BUILD_BIT_KHR is the only valid/documented stage for
    // synchronizing against vkCmdCopyAccelerationStructureKHR as well as builds -
    // VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR requires that feature.
    memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
    memory_barrier.srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
    memory_barrier.dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

    VkDependencyInfo dependency_info = {};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &memory_barrier;

    vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}

}; // namespace

vulkan_ri_raytracing_blas::vulkan_ri_raytracing_blas(vulkan_render_interface& renderer, const char* debug_name)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
{
}

vulkan_ri_raytracing_blas::~vulkan_ri_raytracing_blas()
{
    m_renderer.dequeue_as_build(this);

    destroy_resources();
}

void vulkan_ri_raytracing_blas::destroy_resources()
{
    m_renderer.defer_delete([renderer = &m_renderer, as = m_acceleration_structure, query_pool = m_compaction_query_pool]() mutable
    {
        if (as != VK_NULL_HANDLE)
        {
            renderer->vkDestroyAccelerationStructureKHR_fn(renderer->get_device(), as, nullptr);
        }
        if (query_pool != VK_NULL_HANDLE)
        {
            vkDestroyQueryPool(renderer->get_device(), query_pool, nullptr);
        }
    });

    m_acceleration_structure = VK_NULL_HANDLE;
    m_compaction_query_pool = VK_NULL_HANDLE;

    m_scratch = nullptr;
    m_resource = nullptr;
}

VkAccelerationStructureBuildGeometryInfoKHR vulkan_ri_raytracing_blas::get_input_desc()
{
    m_geometry = {};
    m_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    m_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    m_geometry.flags = 0;
    m_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    m_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    m_geometry.geometry.triangles.vertexData.deviceAddress = static_cast<vulkan_ri_buffer*>(m_build_vertex_buffer)->get_gpu_address();
    m_geometry.geometry.triangles.vertexStride = m_build_vertex_buffer->get_element_size();
    m_geometry.geometry.triangles.maxVertex = static_cast<uint32_t>(m_build_vertex_buffer->get_element_count() - 1);
    m_geometry.geometry.triangles.indexType = (m_build_index_buffer->get_element_size() == 4) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
    m_geometry.geometry.triangles.indexData.deviceAddress = static_cast<vulkan_ri_buffer*>(m_build_index_buffer)->get_gpu_address();
    m_geometry.geometry.triangles.transformData.deviceAddress = 0;

    VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.geometryCount = 1;
    build_info.pGeometries = &m_geometry;

    return build_info;
}

result<void> vulkan_ri_raytracing_blas::create_resources()
{
    destroy_resources();

    if (m_build_index_buffer == nullptr ||
        m_build_vertex_buffer == nullptr)
    {
        return true;
    }

    // Describe the acceleration structure we want to build.
    VkAccelerationStructureBuildGeometryInfoKHR build_info = get_input_desc();
    uint32_t primitive_count = static_cast<uint32_t>(m_build_index_buffer->get_element_count() / 3);

    VkAccelerationStructureBuildSizesInfoKHR size_info = {};
    size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    m_renderer.vkGetAccelerationStructureBuildSizesKHR_fn(
        m_renderer.get_device(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &build_info,
        &primitive_count,
        &size_info
    );

    // Create scratch buffer.
    ri_buffer::create_params scratch_data_params;
    scratch_data_params.element_size = 1;
    scratch_data_params.element_count = math::round_up_multiple(static_cast<size_t>(size_info.buildScratchSize), static_cast<size_t>(256));
    scratch_data_params.usage = ri_buffer_usage::raytracing_as_scratch;
    m_scratch = m_renderer.create_buffer(scratch_data_params, string_format("%s: scratch data", m_debug_name.c_str()).c_str());

    // Create result buffer.
    ri_buffer::create_params result_data_params;
    result_data_params.element_size = 1;
    result_data_params.element_count = math::round_up_multiple(static_cast<size_t>(size_info.accelerationStructureSize), static_cast<size_t>(256));
    result_data_params.usage = ri_buffer_usage::raytracing_as;
    m_resource = m_renderer.create_buffer(result_data_params, string_format("%s: as", m_debug_name.c_str()).c_str());
    m_uncompacted_size = result_data_params.element_count;

    // Create the acceleration structure object, backed by the result buffer.
    VkAccelerationStructureCreateInfoKHR as_create_info = {};
    as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    as_create_info.buffer = static_cast<vulkan_ri_buffer*>(m_resource.get())->get_buffer();
    as_create_info.offset = static_cast<vulkan_ri_buffer*>(m_resource.get())->get_buffer_offset();
    as_create_info.size = size_info.accelerationStructureSize;
    as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    VkResult vk_result = m_renderer.vkCreateAccelerationStructureKHR_fn(m_renderer.get_device(), &as_create_info, nullptr, &m_acceleration_structure);
    if (!m_renderer.check_result(vk_result, "vkCreateAccelerationStructureKHR"))
    {
        return standard_errors::failed;
    }

    // Query pool used to read back the compacted size after building, so we know how large
    // to make the compacted resource in compact().
    VkQueryPoolCreateInfo query_pool_create_info = {};
    query_pool_create_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_create_info.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    query_pool_create_info.queryCount = 1;

    vk_result = vkCreateQueryPool(m_renderer.get_device(), &query_pool_create_info, nullptr, &m_compaction_query_pool);
    if (!m_renderer.check_result(vk_result, "vkCreateQueryPool"))
    {
        return standard_errors::failed;
    }

    return true;
}

void vulkan_ri_raytracing_blas::update(ri_buffer* vertex_buffer, ri_buffer* index_buffer)
{
    m_build_vertex_buffer = vertex_buffer;
    m_build_index_buffer = index_buffer;

    mark_dirty();
}

void vulkan_ri_raytracing_blas::mark_dirty()
{
    if (m_dirty)
    {
        return;
    }

    m_renderer.queue_as_build(this);
    m_dirty = true;
    m_pending_compact = false;
}

void vulkan_ri_raytracing_blas::build(vulkan_ri_command_list& cmd_list)
{
    // Ensure we have appropriately sized resources.
    create_resources();

    if (m_resource == nullptr)
    {
        m_dirty = false;
        return;
    }

    // Transition resources to the states needed for building.
    cmd_list.barrier(*m_scratch, ri_resource_state::initial, ri_resource_state::unordered_access);

    // Dispatch the actual build.
    VkAccelerationStructureBuildGeometryInfoKHR build_info = get_input_desc();
    build_info.dstAccelerationStructure = m_acceleration_structure;
    build_info.scratchData.deviceAddress = static_cast<vulkan_ri_buffer*>(m_scratch.get())->get_gpu_address();

    VkAccelerationStructureBuildRangeInfoKHR range_info = {};
    range_info.primitiveCount = static_cast<uint32_t>(m_build_index_buffer->get_element_count() / 3);
    const VkAccelerationStructureBuildRangeInfoKHR* range_info_ptr = &range_info;

    m_renderer.vkCmdBuildAccelerationStructuresKHR_fn(cmd_list.get_command_buffer(), 1, &build_info, &range_info_ptr);

    acceleration_structure_barrier(cmd_list.get_command_buffer());

    // Query the compacted size, so a later compact() knows how large to make the compacted resource.
    vkCmdResetQueryPool(cmd_list.get_command_buffer(), m_compaction_query_pool, 0, 1);
    m_renderer.vkCmdWriteAccelerationStructuresPropertiesKHR_fn(cmd_list.get_command_buffer(), 1, &m_acceleration_structure, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, m_compaction_query_pool, 0);

    // Transition resources back to what they should be.
    cmd_list.barrier(*m_scratch, ri_resource_state::unordered_access, ri_resource_state::initial);

    // Don't need to keep scratch around anymore.
    m_scratch = nullptr;

    m_is_compacted = false;
    m_pending_compact = true;
    m_build_frame_index = m_renderer.get_frame_index();
    m_dirty = false;

    on_modified.broadcast();
}

bool vulkan_ri_raytracing_blas::is_pending_compaction()
{
    return m_pending_compact;
}

bool vulkan_ri_raytracing_blas::can_compact()
{
    size_t delta = m_renderer.get_frame_index() - m_build_frame_index;
    if (delta <= m_renderer.get_pipeline_depth())
    {
        return false;
    }

    uint64_t compacted_size = 0;
    VkResult vk_result = vkGetQueryPoolResults(
        m_renderer.get_device(),
        m_compaction_query_pool,
        0,
        1,
        sizeof(uint64_t),
        &compacted_size,
        sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT
    );
    if (vk_result != VK_SUCCESS)
    {
        return false;
    }

    m_compacted_size = compacted_size;

    return true;
}

void vulkan_ri_raytracing_blas::compact(vulkan_ri_command_list& cmd_list)
{
    // Recreate destination resource.
    ri_buffer::create_params result_data_params;
    result_data_params.element_size = 1;
    result_data_params.element_count = m_compacted_size;
    result_data_params.usage = ri_buffer_usage::raytracing_as;
    std::unique_ptr<ri_buffer> new_resource = m_renderer.create_buffer(result_data_params, string_format("%s: as compacted", m_debug_name.c_str()).c_str());

    VkAccelerationStructureCreateInfoKHR as_create_info = {};
    as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    as_create_info.buffer = static_cast<vulkan_ri_buffer*>(new_resource.get())->get_buffer();
    as_create_info.offset = static_cast<vulkan_ri_buffer*>(new_resource.get())->get_buffer_offset();
    as_create_info.size = m_compacted_size;
    as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    VkAccelerationStructureKHR new_as = VK_NULL_HANDLE;
    VkResult vk_result = m_renderer.vkCreateAccelerationStructureKHR_fn(m_renderer.get_device(), &as_create_info, nullptr, &new_as);
    if (!m_renderer.check_result(vk_result, "vkCreateAccelerationStructureKHR"))
    {
        return;
    }

    VkCopyAccelerationStructureInfoKHR copy_info = {};
    copy_info.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
    copy_info.src = m_acceleration_structure;
    copy_info.dst = new_as;
    copy_info.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;

    m_renderer.vkCmdCopyAccelerationStructureKHR_fn(cmd_list.get_command_buffer(), &copy_info);

    acceleration_structure_barrier(cmd_list.get_command_buffer());

    m_renderer.defer_delete([renderer = &m_renderer, as = m_acceleration_structure]() mutable
    {
        renderer->vkDestroyAccelerationStructureKHR_fn(renderer->get_device(), as, nullptr);
    });

    m_acceleration_structure = new_as;
    m_resource = std::move(new_resource);
    m_is_compacted = true;
    m_pending_compact = false;

    on_modified.broadcast();
}

VkDeviceAddress vulkan_ri_raytracing_blas::get_gpu_address()
{
    VkAccelerationStructureDeviceAddressInfoKHR address_info = {};
    address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    address_info.accelerationStructure = m_acceleration_structure;

    return m_renderer.vkGetAccelerationStructureDeviceAddressKHR_fn(m_renderer.get_device(), &address_info);
}

const ri_buffer& vulkan_ri_raytracing_blas::get_as_buffer() const
{
    return *m_resource;
}

}; // namespace ws
