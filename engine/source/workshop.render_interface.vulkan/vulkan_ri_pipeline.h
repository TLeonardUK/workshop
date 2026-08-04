// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_pipeline.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>
#include <string>

namespace ws {

class dx12_render_interface;

// ================================================================================================
//  Implementation of a pipeline using DirectX 12.
// ================================================================================================
class dx12_ri_pipeline : public ri_pipeline
{
public:
    dx12_ri_pipeline(dx12_render_interface& renderer, const ri_pipeline::create_params& params, const char* debug_name);
    virtual ~dx12_ri_pipeline();

    // Creates the dx12 resources required by this resource.
    result<void> create_resources();

    ID3D12PipelineState* get_pipeline_state();
    ID3D12StateObject* get_rt_pipeline_state();
    ID3D12RootSignature* get_root_signature();

    virtual const create_params& get_create_params() override;

    bool is_compute();
    bool is_raytracing();

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE get_hit_group_table();
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE get_miss_shader_table();
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE get_ray_generation_shader_record();

protected:

    bool create_root_signature();

    result<void> create_graphics_pso();
    result<void> create_compute_pso();
    result<void> create_raytracing_pso();

    result<void> build_sbt();

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;

    ri_pipeline::create_params m_create_params;

    bool m_is_compute = false;
    bool m_is_raytracing = false;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline_state = nullptr;
    Microsoft::WRL::ComPtr<ID3D12StateObject> m_rt_pipeline_state = nullptr;    
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature = nullptr;

    size_t m_ray_generation_shader_offset = 0;
    size_t m_ray_miss_table_offset = 0;
    size_t m_ray_hit_group_table_offset = 0;

    std::unique_ptr<ri_buffer> m_shader_binding_table;
};

}; // namespace ws
