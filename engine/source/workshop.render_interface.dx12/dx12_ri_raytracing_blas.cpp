// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_raytracing_blas.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"

namespace ws {

dx12_ri_raytracing_blas::dx12_ri_raytracing_blas(dx12_render_interface& renderer, const char* debug_name)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
{
}

dx12_ri_raytracing_blas::~dx12_ri_raytracing_blas()
{
}

void dx12_ri_raytracing_blas::destroy_resources()
{
    m_scratch = nullptr;
    m_resource = nullptr;
}

D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS dx12_ri_raytracing_blas::get_input_desc()
{
    m_geom_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    m_geom_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
    m_geom_desc.Triangles.IndexBuffer = static_cast<dx12_ri_buffer*>(m_build_index_buffer)->get_gpu_address();
    m_geom_desc.Triangles.IndexCount = (UINT)m_build_index_buffer->get_element_count();
    m_geom_desc.Triangles.IndexFormat = m_build_index_buffer->get_element_size() == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    m_geom_desc.Triangles.Transform3x4 = 0;
    m_geom_desc.Triangles.VertexBuffer.StartAddress = static_cast<dx12_ri_buffer*>(m_build_vertex_buffer)->get_gpu_address();
    m_geom_desc.Triangles.VertexBuffer.StrideInBytes = m_build_vertex_buffer->get_element_size();
    m_geom_desc.Triangles.VertexCount = (UINT)m_build_vertex_buffer->get_element_count();
    m_geom_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

    m_input_desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    m_input_desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    m_input_desc.NumDescs = 1;
    m_input_desc.pGeometryDescs = &m_geom_desc;
    m_input_desc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    
    return m_input_desc;
}

result<void> dx12_ri_raytracing_blas::create_resources()
{
    destroy_resources();

    if (m_build_index_buffer == nullptr || 
        m_build_vertex_buffer == nullptr)
    {
        return true;
    }

    // Describe the acceleration structure we want to build.
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuild_desc = get_input_desc();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info;
    m_renderer.get_device()->GetRaytracingAccelerationStructurePrebuildInfo(&prebuild_desc, &prebuild_info);

    // Create scratch buffer.
    ri_buffer::create_params scratch_data_params;
    scratch_data_params.element_size = 1;
    scratch_data_params.element_count = math::round_up_multiple(prebuild_info.ScratchDataSizeInBytes, (UINT64)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    scratch_data_params.usage = ri_buffer_usage::raytracing_as_scratch;
    m_scratch = m_renderer.create_buffer(scratch_data_params, string_format("%s: scratch data", m_debug_name.c_str()).c_str());

    // Create result buffer.
    ri_buffer::create_params result_data_params;
    result_data_params.element_size = 1;
    result_data_params.element_count = math::round_up_multiple(prebuild_info.ResultDataMaxSizeInBytes, (UINT64)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    result_data_params.usage = ri_buffer_usage::raytracing_as;
    m_resource = m_renderer.create_buffer(result_data_params, string_format("%s: as", m_debug_name.c_str()).c_str());

    return true;
}

void dx12_ri_raytracing_blas::update(ri_buffer* vertex_buffer, ri_buffer* index_buffer)
{
    m_build_vertex_buffer = vertex_buffer;
    m_build_index_buffer = index_buffer;

    mark_dirty();
}

void dx12_ri_raytracing_blas::mark_dirty()
{
    if (m_dirty)
    {
        return;
    }

    m_renderer.queue_as_build(this);
    m_dirty = true;
}

void dx12_ri_raytracing_blas::build(dx12_ri_command_list& cmd_list)
{
    // Ensure we have appropriately sized resources.
    create_resources();

    // Transition resources to the states needed for building.
    cmd_list.barrier(*m_scratch, ri_resource_state::initial, ri_resource_state::unordered_access);

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
    cmd_list.barrier_uav(get_resource());

    m_dirty = false;
}

D3D12_GPU_VIRTUAL_ADDRESS dx12_ri_raytracing_blas::get_gpu_address()
{
    return static_cast<dx12_ri_buffer*>(m_resource.get())->get_gpu_address();
}

ID3D12Resource* dx12_ri_raytracing_blas::get_resource()
{
    return static_cast<dx12_ri_buffer*>(m_resource.get())->get_resource();
}

}; // namespace ws
