// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_raytracing_tlas.h"
#include "workshop.render_interface.vulkan/vulkan_ri_raytracing_blas.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_list.h"
#include "workshop.render_interface.vulkan/vulkan_ri_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_table.h"
#include "workshop.render_interface/ri_param_block.h"
#include "workshop.core/math/math.h"
#include "workshop.core/containers/string.h"

#include <cstring>

namespace ws {

namespace {

void acceleration_structure_barrier(VkCommandBuffer command_buffer)
{
    VkMemoryBarrier2 memory_barrier = {};
    memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
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

vulkan_ri_raytracing_tlas::vulkan_ri_raytracing_tlas(vulkan_render_interface& renderer, const char* debug_name)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
{
}

vulkan_ri_raytracing_tlas::~vulkan_ri_raytracing_tlas()
{
    m_renderer.dequeue_as_build(this);

    destroy_resources();
}

void vulkan_ri_raytracing_tlas::destroy_resources()
{
    m_renderer.defer_delete([renderer = &m_renderer, as = m_acceleration_structure]() mutable
    {
        if (as != VK_NULL_HANDLE)
        {
            renderer->vkDestroyAccelerationStructureKHR_fn(renderer->get_device(), as, nullptr);
        }
    });

    m_acceleration_structure = VK_NULL_HANDLE;

    m_scratch = nullptr;
    m_resource = nullptr;
    m_instance_data = nullptr;
}

VkAccelerationStructureBuildGeometryInfoKHR vulkan_ri_raytracing_tlas::get_input_desc()
{
    m_geometry = {};
    m_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    m_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    m_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    m_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    m_geometry.geometry.instances.data.deviceAddress = (m_instance_data == nullptr) ? 0 : static_cast<vulkan_ri_buffer*>(m_instance_data.get())->get_gpu_address();

    VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.geometryCount = 1;
    build_info.pGeometries = &m_geometry;

    return build_info;
}

result<void> vulkan_ri_raytracing_tlas::create_resources()
{
    if (m_instances.size() == 0)
    {
        return true;
    }

    // No need to recreate resources, number of instances remains the same.
    if (m_resource && m_instance_data_size == m_instances.size())
    {
        return true;
    }

    destroy_resources();

    // Describe the acceleration structure we want to build.
    VkAccelerationStructureBuildGeometryInfoKHR build_info = get_input_desc();
    uint32_t primitive_count = static_cast<uint32_t>(m_instances.size());

    VkAccelerationStructureBuildSizesInfoKHR size_info = {};
    size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    m_renderer.vkGetAccelerationStructureBuildSizesKHR_fn(
        m_renderer.get_device(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &build_info,
        &primitive_count,
        &size_info
    );

    // Create instance data buffer.
    ri_buffer::create_params instance_data_params;
    instance_data_params.element_size = sizeof(VkAccelerationStructureInstanceKHR);
    instance_data_params.element_count = m_instances.size();
    instance_data_params.usage = ri_buffer_usage::raytracing_as_instance_data;
    m_instance_data = m_renderer.create_buffer(instance_data_params, string_format("%s: instance data", m_debug_name.c_str()).c_str());

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

    // Create the acceleration structure object, backed by the result buffer.
    VkAccelerationStructureCreateInfoKHR as_create_info = {};
    as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    as_create_info.buffer = static_cast<vulkan_ri_buffer*>(m_resource.get())->get_buffer();
    as_create_info.offset = static_cast<vulkan_ri_buffer*>(m_resource.get())->get_buffer_offset();
    as_create_info.size = size_info.accelerationStructureSize;
    as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    VkResult vk_result = m_renderer.vkCreateAccelerationStructureKHR_fn(m_renderer.get_device(), &as_create_info, nullptr, &m_acceleration_structure);
    if (!m_renderer.check_result(vk_result, "vkCreateAccelerationStructureKHR"))
    {
        return standard_errors::failed;
    }

    // Register the acceleration structure in the bindless tlas descriptor table, so shaders
    // can look it up via the table index stored in a param block field.
    vulkan_ri_buffer* resource_buffer = static_cast<vulkan_ri_buffer*>(m_resource.get());
    m_renderer.get_descriptor_table(ri_descriptor_table::tlas).write_acceleration_structure(resource_buffer->get_srv(), m_acceleration_structure);

    // Create metadata buffer.
    ri_buffer::create_params metadata_params;
    metadata_params.element_size = sizeof(int) * 2;
    metadata_params.element_count = m_instances.size();
    metadata_params.usage = ri_buffer_usage::generic;
    m_metadata_buffer = m_renderer.create_buffer(metadata_params, string_format("%s: metadata", m_debug_name.c_str()).c_str());

    m_instance_data_size = m_instances.size();

    for (size_t i = 0; i < m_instances.size(); i++)
    {
        m_instances[i].dirty = true;
    }

    return true;
}

ri_raytracing_tlas::instance_id vulkan_ri_raytracing_tlas::add_instance(ri_raytracing_blas* blas, const matrix4& transform, size_t domain, bool opaque, ri_param_block* metadata, uint32_t mask)
{
    std::scoped_lock lock(m_instance_mutex);

    size_t index = m_instances.size();
    size_t id = m_next_id;
    m_next_id++;
    m_id_to_index_map[id] = index;

    instance& inst = m_instances.emplace_back();
    inst.blas = static_cast<vulkan_ri_raytracing_blas*>(blas);
    inst.transform = transform;
    inst.domain = domain;
    inst.opaque = opaque;
    inst.metadata = metadata;
    inst.dirty = true;
    inst.mask = mask;
    inst.blas_dirtied_key = inst.blas->on_modified.add_shared([this, id]() {
        mark_instance_dirty(id);
    });

    mark_dirty();

    return id;
}

void vulkan_ri_raytracing_tlas::remove_instance(instance_id id)
{
    std::scoped_lock lock(m_instance_mutex);

    if (auto iter = m_id_to_index_map.find(id); iter != m_id_to_index_map.end())
    {
        size_t index = iter->second;
        m_id_to_index_map.erase(iter);

        m_instances.erase(m_instances.begin() + index);

        // Go through every instance, everything with an index above the removed element needs to be
        // shuffled back by one to match its new index.
        for (auto& pair : m_id_to_index_map)
        {
            if (pair.second > index)
            {
                pair.second--;
                m_instances[pair.second].dirty = true;
            }
        }
    }

    mark_dirty();
}

void vulkan_ri_raytracing_tlas::mark_instance_dirty(instance_id id)
{
    std::scoped_lock lock(m_instance_mutex);

    if (auto iter = m_id_to_index_map.find(id); iter != m_id_to_index_map.end())
    {
        instance& inst = m_instances[iter->second];
        inst.dirty = true;
    }

    mark_dirty();
}

void vulkan_ri_raytracing_tlas::update_instance(instance_id id, const matrix4& transform, uint32_t mask)
{
    std::scoped_lock lock(m_instance_mutex);

    if (auto iter = m_id_to_index_map.find(id); iter != m_id_to_index_map.end())
    {
        instance& inst = m_instances[iter->second];
        inst.transform = transform;
        inst.mask = mask;
        inst.dirty = true;
    }

    mark_dirty();
}

ri_buffer* vulkan_ri_raytracing_tlas::get_metadata_buffer() const
{
    return m_metadata_buffer.get();
}

const ri_buffer& vulkan_ri_raytracing_tlas::get_tlas_buffer() const
{
    return *m_resource;
}

void vulkan_ri_raytracing_tlas::mark_dirty()
{
    if (m_dirty)
    {
        return;
    }

    m_renderer.queue_as_build(this);
    m_dirty = true;
}

void vulkan_ri_raytracing_tlas::build(vulkan_ri_command_list& cmd_list)
{
    size_t ray_type_count = m_renderer.get_ray_type_count();

    // Ensure we have appropriately sized resources.
    create_resources();

    if (m_resource == nullptr)
    {
        m_dirty = false;
        return;
    }

    // Update instance data.
    for (size_t i = 0; i < m_instances.size(); i++)
    {
        instance& inst = m_instances[i];
        if (inst.dirty)
        {
            // Update TLAS instance data.
            VkAccelerationStructureInstanceKHR* desc = static_cast<VkAccelerationStructureInstanceKHR*>(m_instance_data->map(i * sizeof(VkAccelerationStructureInstanceKHR), sizeof(VkAccelerationStructureInstanceKHR)));
            memcpy(&desc->transform, &inst.transform, sizeof(desc->transform));
            desc->instanceCustomIndex = static_cast<uint32_t>(i);
            desc->mask = inst.mask;
            desc->instanceShaderBindingTableRecordOffset = static_cast<uint32_t>(inst.domain * ray_type_count);
            desc->flags = inst.opaque ? VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR : VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;
            desc->accelerationStructureReference = inst.blas->get_gpu_address();
            m_instance_data->unmap(desc);

            // Update metadata buffer.
            size_t table_index, table_offset;
            inst.metadata->get_table(table_index, table_offset);

            uint32_t* indexes = reinterpret_cast<uint32_t*>(m_metadata_buffer->map(i * sizeof(uint32_t) * 2, sizeof(uint32_t) * 2));
            indexes[0] = static_cast<uint32_t>(table_index);
            indexes[1] = static_cast<uint32_t>(table_offset);
            m_metadata_buffer->unmap(indexes);

            inst.dirty = false;
        }
    }

    // Note: We just rebuild the entire TLAS rather than updating it. The documentation suggests
    // this is more efficient as rebuilding the tlas takes minimal time.

    // Transition resources to the states needed for building.
    cmd_list.barrier(*m_scratch, ri_resource_state::initial, ri_resource_state::unordered_access);
    cmd_list.barrier(*m_instance_data, ri_resource_state::initial, ri_resource_state::non_pixel_shader_resource);

    // Dispatch the actual build.
    VkAccelerationStructureBuildGeometryInfoKHR build_info = get_input_desc();
    build_info.dstAccelerationStructure = m_acceleration_structure;
    build_info.scratchData.deviceAddress = static_cast<vulkan_ri_buffer*>(m_scratch.get())->get_gpu_address();

    VkAccelerationStructureBuildRangeInfoKHR range_info = {};
    range_info.primitiveCount = static_cast<uint32_t>(m_instances.size());
    const VkAccelerationStructureBuildRangeInfoKHR* range_info_ptr = &range_info;

    m_renderer.vkCmdBuildAccelerationStructuresKHR_fn(cmd_list.get_command_buffer(), 1, &build_info, &range_info_ptr);

    acceleration_structure_barrier(cmd_list.get_command_buffer());

    // Transition resources back to what they should be.
    cmd_list.barrier(*m_scratch, ri_resource_state::unordered_access, ri_resource_state::initial);
    cmd_list.barrier(*m_instance_data, ri_resource_state::non_pixel_shader_resource, ri_resource_state::initial);

    m_dirty = false;
}

}; // namespace ws
