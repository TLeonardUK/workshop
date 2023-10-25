// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_raytracing_tlas.h"
#include "workshop.render_interface.dx12/dx12_ri_raytracing_blas.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_param_block.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"

namespace ws {

dx12_ri_raytracing_tlas::dx12_ri_raytracing_tlas(dx12_render_interface& renderer, const char* debug_name)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
{
}

dx12_ri_raytracing_tlas::~dx12_ri_raytracing_tlas()
{
    destroy_resources();
}

void dx12_ri_raytracing_tlas::destroy_resources()
{
    m_scratch = nullptr;
    m_resource = nullptr;
    m_instance_data = nullptr;
}

D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS dx12_ri_raytracing_tlas::get_input_desc()
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuild_desc = {};
    prebuild_desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    prebuild_desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    prebuild_desc.NumDescs = static_cast<UINT>(m_instances.size());
    prebuild_desc.InstanceDescs = m_instance_data == nullptr ? 0 : static_cast<dx12_ri_buffer*>(m_instance_data.get())->get_gpu_address();
    prebuild_desc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    return prebuild_desc;
}

result<void> dx12_ri_raytracing_tlas::create_resources()
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
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS input_desc = get_input_desc();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = {};
    m_renderer.get_device()->GetRaytracingAccelerationStructurePrebuildInfo(&input_desc, &prebuild_info);

    prebuild_info.ResultDataMaxSizeInBytes = math::round_up_multiple(prebuild_info.ResultDataMaxSizeInBytes, (UINT64)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    prebuild_info.ScratchDataSizeInBytes = math::round_up_multiple(prebuild_info.ScratchDataSizeInBytes, (UINT64)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    prebuild_info.UpdateScratchDataSizeInBytes = math::round_up_multiple(prebuild_info.UpdateScratchDataSizeInBytes, (UINT64)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    // Create instance data buffer.
    ri_buffer::create_params instance_data_params;
    instance_data_params.element_size = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
    instance_data_params.element_count = m_instances.size();
    instance_data_params.usage = ri_buffer_usage::raytracing_as_instance_data;
    m_instance_data = m_renderer.create_buffer(instance_data_params, string_format("%s: instance data", m_debug_name.c_str()).c_str());

    // Create scratch buffer.
    ri_buffer::create_params scratch_data_params;
    scratch_data_params.element_size = 1;
    scratch_data_params.element_count = prebuild_info.ScratchDataSizeInBytes;
    scratch_data_params.usage = ri_buffer_usage::raytracing_as_scratch;
    m_scratch = m_renderer.create_buffer(scratch_data_params, string_format("%s: scratch data", m_debug_name.c_str()).c_str());

    // Create result buffer.
    ri_buffer::create_params result_data_params;
    result_data_params.element_size = 1;
    result_data_params.element_count = prebuild_info.ResultDataMaxSizeInBytes;
    result_data_params.usage = ri_buffer_usage::raytracing_as;
    m_resource = m_renderer.create_buffer(result_data_params, string_format("%s: as", m_debug_name.c_str()).c_str());

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

dx12_ri_raytracing_tlas::instance_id dx12_ri_raytracing_tlas::add_instance(ri_raytracing_blas* blas, const matrix4& transform, size_t domain, bool opaque, ri_param_block* metadata)
{
    std::scoped_lock lock(m_instance_mutex);

    size_t index = m_instances.size();
    size_t id = m_next_id;
    m_next_id++;
    m_id_to_index_map[id] = index;

    instance& inst = m_instances.emplace_back();
    inst.blas = static_cast<dx12_ri_raytracing_blas*>(blas);
    inst.transform = transform;
    inst.domain = domain;
    inst.opaque = opaque;
    inst.metadata = metadata;
    inst.dirty = true;

    mark_dirty();

    return id;
}

void dx12_ri_raytracing_tlas::remove_instance(instance_id id)
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

void dx12_ri_raytracing_tlas::update_instance(instance_id id, const matrix4& transform)
{
    std::scoped_lock lock(m_instance_mutex);

    if (auto iter = m_id_to_index_map.find(id); iter != m_id_to_index_map.end())
    {
        instance& inst = m_instances[iter->second];
        inst.transform = transform;
        inst.dirty = true;
    }

    mark_dirty();
}

ri_buffer* dx12_ri_raytracing_tlas::get_metadata_buffer()
{
    return m_metadata_buffer.get();
}

void dx12_ri_raytracing_tlas::mark_dirty()
{
    if (m_dirty)
    {
        return;
    }

    m_renderer.queue_as_build(this);
    m_dirty = true;
}

void dx12_ri_raytracing_tlas::build(dx12_ri_command_list& cmd_list)
{
    // Ensure we have appropriately sized resources.
    create_resources();

    // Update instance data.
    for (size_t i = 0; i < m_instances.size(); i++)
    {
        instance& inst = m_instances[i];
        if (inst.dirty)
        {
            // Update TLAS instance data.
            D3D12_RAYTRACING_INSTANCE_DESC* desc = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(m_instance_data->map(i * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), sizeof(D3D12_RAYTRACING_INSTANCE_DESC)));
            desc->AccelerationStructure = inst.blas->get_gpu_address();
            memcpy(desc->Transform, &inst.transform, sizeof(desc->Transform));
            desc->Flags = inst.opaque ? D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE : D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
            desc->InstanceMask = 0xFF;
            desc->InstanceID = i;
            desc->InstanceContributionToHitGroupIndex = inst.domain;
            m_instance_data->unmap(desc);

            // Update metadata buffer.
            size_t table_index, table_offset;
            inst.metadata->get_table(table_index, table_offset);

            uint32_t* indexes = reinterpret_cast<uint32_t*>(m_metadata_buffer->map(i * sizeof(uint32_t) * 2, sizeof(uint32_t) * 2));
            indexes[0] = (uint32_t)table_index;
            indexes[1] = (uint32_t)table_offset;
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
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = {};
    build_desc.ScratchAccelerationStructureData = static_cast<dx12_ri_buffer*>(m_scratch.get())->get_gpu_address();
    build_desc.DestAccelerationStructureData = static_cast<dx12_ri_buffer*>(m_resource.get())->get_gpu_address();
    build_desc.Inputs = get_input_desc();

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> cmd_list_5 = nullptr;
    cmd_list.get_dx_command_list().As<ID3D12GraphicsCommandList5>(&cmd_list_5);
    cmd_list_5->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);
    
    // Transition resources back to what they should be.
    cmd_list.barrier(*m_scratch, ri_resource_state::unordered_access, ri_resource_state::initial);
    cmd_list.barrier(*m_instance_data, ri_resource_state::non_pixel_shader_resource, ri_resource_state::initial);

    m_dirty = false;
}

}; // namespace ws
