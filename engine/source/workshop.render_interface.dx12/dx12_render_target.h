// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/render_target.h"
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

// ================================================================================================
//  Implementation of a command queue using DirectX 12.
// ================================================================================================
class dx12_render_target : public render_target
{
public:
    dx12_render_target(dx12_render_interface& renderer, const char* debug_name, Microsoft::WRL::ComPtr<ID3D12Resource> buffer);
    virtual ~dx12_render_target();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    Microsoft::WRL::ComPtr<ID3D12Resource> get_buffer();
    D3D12_CPU_DESCRIPTOR_HANDLE get_rtv();

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtv = {};

};

}; // namespace ws
