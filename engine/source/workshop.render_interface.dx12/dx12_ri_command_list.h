// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_command_list.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"

#include <array>
#include <string>
#include <vector>

namespace ws {

class engine;
class ri_param_block;
class dx12_render_interface;
class dx12_ri_command_queue;
class dx12_ri_pipeline;

// ================================================================================================
//  Implementation of a command list using DirectX 12.
// ================================================================================================
class dx12_ri_command_list : public ri_command_list
{
public:
    dx12_ri_command_list(dx12_render_interface& renderer, const char* debug_name, dx12_ri_command_queue& queue);
    virtual ~dx12_ri_command_list();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    virtual void open() override;
    virtual void close() override;
    virtual void barrier(ri_texture& resource, ri_resource_state source_state, ri_resource_state destination_state) override;
    virtual void barrier(ri_buffer& resource, ri_resource_state source_state, ri_resource_state destination_state) override;
    virtual void clear(ri_texture& resource, const color& destination) override;
    virtual void clear_depth(ri_texture& resource, float depth, size_t stencil) override;
    virtual void set_pipeline(ri_pipeline& pipeline) override;
    virtual void set_param_blocks(const std::vector<ri_param_block*> param_blocks) override;
    virtual void set_viewport(const recti& rect) override;
    virtual void set_scissor(const recti& rect) override;
    virtual void set_blend_factor(const vector4& factor) override;
    virtual void set_stencil_ref(uint32_t value) override;
    virtual void set_primitive_topology(ri_primitive value) override;
    virtual void set_index_buffer(ri_buffer& buffer) override;
    virtual void set_render_targets(const std::vector<ri_texture*>& colors, ri_texture* depth) override;
    virtual void draw(size_t indexes_per_instance, size_t instance_count, size_t start_index_location) override;
    virtual void begin_event(const color& color, const char* name, ...) override;
    virtual void end_event() override;

    void barrier(ID3D12Resource* resource, ri_resource_state resource_initial_state, ri_resource_state source_state, ri_resource_state destination_state);

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> get_dx_command_list();

    bool is_open();
    void set_allocated_frame(size_t frame);

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;
    dx12_ri_command_queue& m_queue;

    bool m_opened = false;
    size_t m_allocated_frame_index = 0;

    dx12_ri_pipeline* m_active_pipeline = nullptr;

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list;

};

}; // namespace ws
