// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/render_fence.h"
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
//  Implementation of a fence using DirectX 12.
// ================================================================================================
class dx12_render_fence : public render_fence
{
public:
    dx12_render_fence(dx12_render_interface& renderer, const char* debug_name);
    virtual ~dx12_render_fence();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    virtual void signal(size_t value) override;
    virtual void wait(size_t value) override;
    virtual size_t current_value() override;
    virtual void signal(render_command_queue& queue, size_t value) override;

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;

    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence = nullptr;
    HANDLE m_fence_event = INVALID_HANDLE_VALUE;

};

}; // namespace ws
