// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_render_swapchain.h"
#include "workshop.render_interface.dx12/dx12_render_interface.h"
#include "workshop.render_interface.dx12/dx12_render_command_queue.h"
#include "workshop.render_interface.dx12/dx12_render_fence.h"
#include "workshop.render_interface.dx12/dx12_render_descriptor_heap.h"
#include "workshop.render_interface.dx12/dx12_render_target.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.core/perf/profile.h"
#include "workshop.windowing/window.h"

namespace ws {

dx12_render_swapchain::dx12_render_swapchain(dx12_render_interface& renderer, window& for_window, const char* debug_name)
    : m_renderer(renderer)
    , m_window(for_window)
    , m_debug_name(debug_name)
{
}

dx12_render_swapchain::~dx12_render_swapchain()
{
    destroy_resources();
}

void dx12_render_swapchain::destroy_resources()
{
    drain();

    for (size_t i = 0; i < dx12_render_interface::k_max_pipeline_depth; i++)
    {
        m_back_buffer_targets[i] = nullptr;
        m_back_buffer_last_used_frame[i] = 0;
    }
    SafeRelease(m_swap_chain);
}

result<void> dx12_render_swapchain::create_resources()
{
    HRESULT hr;

    HWND hwnd = reinterpret_cast<HWND>(m_window.get_platform_handle());

    dx12_render_command_queue& graphics_command_queue = static_cast<dx12_render_command_queue&>(
        m_renderer.get_graphics_queue()
    );

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width = static_cast<UINT>(m_window.get_width());
    swap_chain_desc.Height = static_cast<UINT>(m_window.get_height());
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.Stereo = FALSE;
    swap_chain_desc.SampleDesc = { 1, 0 };
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = dx12_render_interface::k_max_pipeline_depth;
    swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc.Flags = m_renderer.is_tearing_allowed() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain;

    // Create dx swapchain.
    hr = m_renderer.get_dxgi_factory()->CreateSwapChainForHwnd(
        graphics_command_queue.get_queue().Get(),
        hwnd,
        &swap_chain_desc,
        nullptr,
        nullptr,
        &swap_chain
    );
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateSwapChainForHwnd failed with error 0x%08x.", hr);
        return false;
    }

    hr = m_renderer.get_dxgi_factory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(hr))
    {
        db_error(render_interface, "MakeWindowAssociation failed with error 0x%08x.", hr);
        return false;
    }

    hr = swap_chain.As(&m_swap_chain);
    if (FAILED(hr))
    {
        db_error(render_interface, "Failed to get IDXGISwapChain4 with error 0x%08x.", hr);
        return false;
    }

    // Create RTV views of each swapchain buffer.
    if (!create_render_targets())
    {
        return false;
    }

    m_fence = m_renderer.create_fence(string_format("%s - Swap Chain Fence", m_debug_name.c_str()).c_str());
    if (!m_fence)
    {
        return false;
    }

    m_window_width = m_window.get_width();
    m_window_height = m_window.get_height();
    m_window_mode = m_window.get_mode();

    return true;
}

result<void> dx12_render_swapchain::create_render_targets()
{
    for (size_t i = 0; i < dx12_render_interface::k_max_pipeline_depth; i++)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> buffer;

        HRESULT hr = m_swap_chain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&buffer));
        if (FAILED(hr))
        {
            db_error(render_interface, "IDXGISwapChain4::GetBuffer failed with error 0x%08x.", hr);
            return false;
        }

        std::string buffer_name = string_format("%s[%i]", m_debug_name.c_str(), i);
        m_back_buffer_last_used_frame[i] = 0;
        m_back_buffer_targets[i] = std::make_unique<dx12_render_target>(m_renderer, buffer_name.c_str(), buffer);
        if (!m_back_buffer_targets[i]->create_resources())
        {
            return false;
        }
    }

    return true;
}

result<void> dx12_render_swapchain::resize_buffers()
{
    for (size_t i = 0; i < dx12_render_interface::k_max_pipeline_depth; i++)
    {
        m_back_buffer_targets[i] = nullptr;
        m_back_buffer_last_used_frame[i] = 0;
    }

    HRESULT hr = m_swap_chain->ResizeBuffers(
        dx12_render_interface::k_max_pipeline_depth,
        static_cast<UINT>(m_window.get_width()),
        static_cast<UINT>(m_window.get_height()),
        DXGI_FORMAT_R8G8B8A8_UNORM,
        m_renderer.is_tearing_allowed() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0
    );

    if (FAILED(hr))
    {
        db_error(render_interface, "Failed to ResizeBuffers with error 0x%08x.", hr);
        return false;
    }

    if (!create_render_targets())
    {
        return false;
    }

    m_window_width = m_window.get_width();
    m_window_height = m_window.get_height();
    m_window_mode = m_window.get_mode();

    return true;
}

render_target& dx12_render_swapchain::next_backbuffer()
{
    m_current_buffer_index = m_swap_chain->GetCurrentBackBufferIndex();

    // If back buffer has been used in the past ensure gpu has
    // finished with it.
    size_t last_frame_used = m_back_buffer_last_used_frame[m_current_buffer_index];
    if (last_frame_used > 0)
    {
        profile_marker(profile_colors::wait, "wait for gpu");
        m_fence->wait(last_frame_used);
    }

    return *m_back_buffer_targets[m_current_buffer_index];
}

void dx12_render_swapchain::present()
{
    profile_marker(profile_colors::wait, "present");

    HRESULT hr = m_swap_chain->Present(0, m_renderer.is_tearing_allowed() ? DXGI_PRESENT_ALLOW_TEARING : 0);
    if (FAILED(hr))
    {
        db_error(render_interface, "Present failed with error 0x%08x.", hr);
    }

    // Signal last back buffer is complete.
    m_back_buffer_last_used_frame[m_current_buffer_index] = m_frame_index;
    
    m_fence->signal(m_renderer.get_graphics_queue(), m_frame_index);

    m_frame_index++;

    // If window has changed size, we need to regenerate the swapchain.
    if (m_window.get_width() != m_window_width ||
        m_window.get_height() != m_window_height ||
        m_window.get_mode() != m_window_mode)
    {
        db_log(render_interface, "Window metrics changed, recreating swapchain.");

        drain();
        if (!resize_buffers())
        {
            db_fatal(render_interface, "Failed to recreate swapchain.");
        }
    }
}

void dx12_render_swapchain::drain()
{
    if (m_frame_index > 0)
    {
        profile_marker(profile_colors::wait, "draining gpu");
        m_fence->wait(m_frame_index - 1);
    }
}

}; // namespace ws
