// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_fence.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Implementation of a fence using DirectX 12.
// ================================================================================================
class dx12_ri_fence : public ri_fence
{
public:
    dx12_ri_fence(dx12_render_interface& renderer, const char* debug_name);
    virtual ~dx12_ri_fence();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    virtual void wait(size_t value) override;
    virtual void wait(ri_command_queue& queue, size_t value) override;
    virtual size_t current_value() override;
    virtual void signal(size_t value) override;
    virtual void signal(ri_command_queue& queue, size_t value) override;

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;

    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence = nullptr;
    HANDLE m_fence_event = INVALID_HANDLE_VALUE;

};

}; // namespace ws
