// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/render_interface.h"
#include "workshop.render_interface.dx12/dx12_render_descriptor_heap.h"

#include "workshop.core.win32/utils/windows_headers.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>

namespace ws {

class dx12_render_command_queue;

// ================================================================================================
//  Implementation of a renderer using DirectX 12.
// ================================================================================================
class dx12_render_interface : public render_interface
{
public:
    constexpr static size_t k_max_pipeline_depth = 3;
 
    dx12_render_interface();
    virtual ~dx12_render_interface();

    virtual void register_init(init_list& list) override;
    virtual void new_frame() override;
    virtual std::unique_ptr<render_swapchain> create_swapchain(window& for_window, const char* debug_name) override;
    virtual std::unique_ptr<render_fence> create_fence(const char* debug_name) override;
    virtual render_command_queue& get_graphics_queue() override;
    virtual size_t get_pipeline_depth() override;

    bool is_tearing_allowed();
    Microsoft::WRL::ComPtr<IDXGIFactory4> get_dxgi_factory();
    Microsoft::WRL::ComPtr<ID3D12Device> get_device();

    dx12_render_descriptor_heap& get_uav_descriptor_heap();
    dx12_render_descriptor_heap& get_sampler_descriptor_heap();
    dx12_render_descriptor_heap& get_rtv_descriptor_heap();
    dx12_render_descriptor_heap& get_dsv_descriptor_heap();

    size_t get_frame_index();

private:

    result<void> create_device();
    result<void> destroy_device();

    result<void> create_command_queues();
    result<void> destroy_command_queues();

    result<void> create_command_list();
    result<void> destroy_command_list();

    result<void> create_heaps();
    result<void> destroy_heaps();

    result<void> select_adapter();
    result<void> check_feature_support();

private:

    Microsoft::WRL::ComPtr<ID3D12Device> m_device = nullptr;

    std::unique_ptr<dx12_render_command_queue> m_graphics_queue = nullptr;

    std::unique_ptr<dx12_render_descriptor_heap> m_uav_descriptor_heap = nullptr;
    std::unique_ptr<dx12_render_descriptor_heap> m_sampler_descriptor_heap = nullptr;
    std::unique_ptr<dx12_render_descriptor_heap> m_rtv_descriptor_heap = nullptr;
    std::unique_ptr<dx12_render_descriptor_heap> m_dsv_descriptor_heap = nullptr;

    Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgi_factory = nullptr;
    Microsoft::WRL::ComPtr<IDXGIFactory5> m_dxgi_factory_5 = nullptr;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> m_dxgi_adapter = nullptr;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> m_info_queue = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Debug> m_debug_interface = nullptr;

    D3D12_FEATURE_DATA_D3D12_OPTIONS m_options = {};
    bool m_allow_tearing = false;

    size_t m_frame_index = 0;

};

}; // namespace ws
