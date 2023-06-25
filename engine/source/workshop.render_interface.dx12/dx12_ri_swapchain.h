// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_swapchain.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.window_interface/window.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>

namespace ws {

class engine;
class window;
class ri_fence;
class dx12_render_interface;
class dx12_ri_texture;

// ================================================================================================
//  Implementation of a swapchain using DirectX 12.
// ================================================================================================
class dx12_ri_swapchain : public ri_swapchain
{
public:
    dx12_ri_swapchain(dx12_render_interface& renderer, window& for_window, const char* debug_name);
    virtual ~dx12_ri_swapchain();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    virtual ri_texture& next_backbuffer() override;
    virtual void present() override;
    virtual void drain() override;

protected:
    void destroy_resources();
    result<void> resize_buffers();
    result<void> create_render_targets();

private:    
    // Minus one to account for the frame currently being generated.
    constexpr static size_t k_buffer_count = dx12_render_interface::k_max_pipeline_depth - 1;

    std::string m_debug_name;
    dx12_render_interface& m_renderer;
    window& m_window;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain = nullptr;
    std::array<std::unique_ptr<dx12_ri_texture>, k_buffer_count> m_back_buffer_targets = {};

    std::unique_ptr<ri_fence> m_fence;

    std::array<size_t, k_buffer_count> m_back_buffer_last_used_frame = {};
    size_t m_current_buffer_index = 0;
    size_t m_frame_index = 1;

    size_t m_window_width = 0;
    size_t m_window_height = 0;
    window_mode m_window_mode = window_mode::windowed;

};

}; // namespace ws
