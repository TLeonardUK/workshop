// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/render_command_list.h"
#include "workshop.core/utils/result.h"
#include "workshop.core.win32/utils/windows_headers.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;
class dx12_render_command_queue;

// ================================================================================================
//  Implementation of a command list using DirectX 12.
// ================================================================================================
class dx12_render_command_list : public render_command_list
{
public:
    dx12_render_command_list(dx12_render_interface& renderer, const char* debug_name, dx12_render_command_queue& queue);
    virtual ~dx12_render_command_list();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    virtual void open() override;
    virtual void close() override;
    virtual void barrier(render_target& resource, render_resource_state source_state, render_resource_state destination_state) override;
    virtual void clear(render_target& resource, const color& destination) override;

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> get_dx_command_list();

    bool is_open();
    void set_allocated_frame(size_t frame);

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;
    dx12_render_command_queue& m_queue;

    bool m_opened = false;
    size_t m_allocated_frame_index = 0;

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list;

};

}; // namespace ws
