// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_raytracing_blas.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"
#include "workshop.render_interface.dx12/dx12_ri_fence.h"
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
    m_renderer.dequeue_as_build(this);

    destroy_resources();
}

void dx12_ri_raytracing_blas::destroy_resources()
{
    m_scratch = nullptr;
    m_resource = nullptr;
    m_compacted_size_buffer = nullptr;
    m_compacted_size_readback_buffer = nullptr;
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
    m_input_desc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
    
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
    m_uncompacted_size = result_data_params.element_count;

    // Create buffer to store compacted size in.
    ri_buffer::create_params compacted_size_params;
    compacted_size_params.element_size = 1;
    compacted_size_params.element_count = sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC);
    compacted_size_params.usage = ri_buffer_usage::readback;
    m_compacted_size_readback_buffer = m_renderer.create_buffer(compacted_size_params, string_format("%s: compaction size readback", m_debug_name.c_str()).c_str());

    compacted_size_params.usage = ri_buffer_usage::generic;
    m_compacted_size_buffer = m_renderer.create_buffer(compacted_size_params, string_format("%s: compaction size", m_debug_name.c_str()).c_str());

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
    m_pending_compact = false;
}

void dx12_ri_raytracing_blas::build(dx12_ri_command_list& cmd_list)
{
    // Ensure we have appropriately sized resources.
    create_resources();

    // Transition resources to the states needed for building.
    cmd_list.barrier(*m_scratch, ri_resource_state::initial, ri_resource_state::unordered_access);
    cmd_list.barrier(*m_compacted_size_buffer, ri_resource_state::initial, ri_resource_state::unordered_access);

    // Dispatch the actual build.
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = {};
    build_desc.ScratchAccelerationStructureData = static_cast<dx12_ri_buffer*>(m_scratch.get())->get_gpu_address();
    build_desc.DestAccelerationStructureData = static_cast<dx12_ri_buffer*>(m_resource.get())->get_gpu_address();
    build_desc.Inputs = get_input_desc();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC post_build_desc;
    post_build_desc.DestBuffer = static_cast<dx12_ri_buffer*>(m_compacted_size_buffer.get())->get_gpu_address();
    post_build_desc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> cmd_list_5 = nullptr;
    cmd_list.get_dx_command_list().As<ID3D12GraphicsCommandList5>(&cmd_list_5);
    cmd_list_5->BuildRaytracingAccelerationStructure(&build_desc, 1, &post_build_desc);

    // Copy the size back to our readback buffer.
    dx12_ri_buffer* dx_compact_readback_buffer = static_cast<dx12_ri_buffer*>(m_compacted_size_readback_buffer.get());
    dx12_ri_buffer* dx_compact_buffer = static_cast<dx12_ri_buffer*>(m_compacted_size_buffer.get());

    cmd_list.barrier(*m_compacted_size_buffer, ri_resource_state::unordered_access, ri_resource_state::copy_source);
    cmd_list_5->CopyBufferRegion(
        dx_compact_readback_buffer->get_resource(),
        dx_compact_readback_buffer->get_buffer_offset(),
        dx_compact_buffer->get_resource(),
        dx_compact_buffer->get_buffer_offset(),
        sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC)
    );
    cmd_list.barrier(*m_compacted_size_buffer, ri_resource_state::copy_source, ri_resource_state::initial);

    // Transition resources back to what they should be.
    cmd_list.barrier(*m_scratch, ri_resource_state::unordered_access, ri_resource_state::initial);
    cmd_list.barrier_uav(get_resource());

    // Don't need to keep scratch around anymore.
    m_scratch = nullptr;

    m_is_compacted = false;
    m_pending_compact = true;
    m_build_frame_index = m_renderer.get_frame_index();
    m_dirty = false;

    on_modified.broadcast();
}

bool dx12_ri_raytracing_blas::is_pending_compaction()
{
    return m_pending_compact;
}

bool dx12_ri_raytracing_blas::can_compact()
{
    size_t delta = m_renderer.get_frame_index() - m_build_frame_index;
    if (delta <= m_renderer.get_pipeline_depth())
    {
        return false;
    }

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC* ptr = (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC*)m_compacted_size_readback_buffer->map(0, sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC));
    m_compacted_size = ptr->CompactedSizeInBytes;
    m_compacted_size_readback_buffer->unmap(ptr);

    return true;
}

void dx12_ri_raytracing_blas::compact(dx12_ri_command_list& cmd_list)
{
    // Recreate destination resource.
    ri_buffer::create_params result_data_params;
    result_data_params.element_size = 1;
    result_data_params.element_count = m_compacted_size;
    result_data_params.usage = ri_buffer_usage::raytracing_as;
    std::unique_ptr<ri_buffer> new_resource = m_renderer.create_buffer(result_data_params, string_format("%s: as compacted", m_debug_name.c_str()).c_str());

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> cmd_list_5 = nullptr;
    cmd_list.get_dx_command_list().As<ID3D12GraphicsCommandList5>(&cmd_list_5);

    cmd_list_5->CopyRaytracingAccelerationStructure(
        static_cast<dx12_ri_buffer*>(new_resource.get())->get_gpu_address(),
        static_cast<dx12_ri_buffer*>(m_resource.get())->get_gpu_address(),
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT
    );

    cmd_list.barrier_uav(get_resource());
    
    m_resource = std::move(new_resource);
    m_is_compacted = true;
    m_pending_compact = false;
    
    on_modified.broadcast();
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
