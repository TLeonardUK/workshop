// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_raytracing_blas.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;
class dx12_ri_command_list;

// ================================================================================================
//  Implementation of a bottom level acceleration structure for DirectX 12.
// ================================================================================================
class dx12_ri_raytracing_blas : public ri_raytracing_blas
{
public:
    dx12_ri_raytracing_blas(dx12_render_interface& renderer, const char* debug_name);
    virtual ~dx12_ri_raytracing_blas();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();
    void destroy_resources();

    virtual void update(ri_buffer* vertex_buffer, ri_buffer* index_buffer) override;

    // Called by interface each frame if building is required.
    void build(dx12_ri_command_list& cmd_list);

    D3D12_GPU_VIRTUAL_ADDRESS get_gpu_address();

    ID3D12Resource* get_resource();

private:
    void mark_dirty();

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS get_input_desc();

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;

    std::unique_ptr<ri_buffer> m_scratch;
    std::unique_ptr<ri_buffer> m_resource;

    ri_buffer* m_build_vertex_buffer = nullptr;
    ri_buffer* m_build_index_buffer = nullptr;

    D3D12_RAYTRACING_GEOMETRY_DESC m_geom_desc;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS m_input_desc;

    bool m_dirty = false;

};

}; // namespace ws
