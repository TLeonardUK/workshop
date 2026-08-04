// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_raytracing_tlas.h"
#include "workshop.core/utils/result.h"
#include "workshop.core/utils/event.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;
class dx12_ri_command_list;
class dx12_ri_raytracing_blas;

// ================================================================================================
//  Implementation of a top level acceleration structure for DirectX 12.
// ================================================================================================
class dx12_ri_raytracing_tlas : public ri_raytracing_tlas
{
public:
    dx12_ri_raytracing_tlas(dx12_render_interface& renderer, const char* debug_name);
    virtual ~dx12_ri_raytracing_tlas();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();
    void destroy_resources();

    virtual instance_id add_instance(ri_raytracing_blas* blas, const matrix4& transform, size_t domain, bool opaque, ri_param_block* metadata, uint32_t mask) override;
    virtual void remove_instance(instance_id id) override;
    virtual void update_instance(instance_id id, const matrix4& transform, uint32_t mask) override;
    virtual ri_buffer* get_metadata_buffer() override;

    void mark_instance_dirty(instance_id id);

    const ri_buffer& get_tlas_buffer() const;

    // Called by interface each frame if building is required.
    void build(dx12_ri_command_list& cmd_list);

private:
    void mark_dirty();

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS get_input_desc();

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;

    struct instance
    {
        dx12_ri_raytracing_blas* blas;
        event<>::delegate_ptr blas_dirtied_key;
        matrix4 transform;
        size_t domain;
        ri_param_block* metadata;
        bool opaque;
        bool dirty;
        uint32_t mask;
    };

    std::mutex m_instance_mutex;

    std::vector<instance> m_instances;
    std::unordered_map<instance_id, size_t> m_id_to_index_map;
    size_t m_instance_data_size = 0;
    instance_id m_next_id = 0;

    std::unique_ptr<ri_buffer> m_scratch;
    std::unique_ptr<ri_buffer> m_resource;
    std::unique_ptr<ri_buffer> m_instance_data;
    std::unique_ptr<ri_buffer> m_metadata_buffer;

    std::vector<size_t> m_active_instance_indices;

    bool m_dirty = false;

};

}; // namespace ws
