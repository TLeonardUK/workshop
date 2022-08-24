// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_render_interface.h"
#include "workshop.render_interface.dx12/dx12_render_swapchain.h"
#include "workshop.render_interface.dx12/dx12_render_fence.h"
#include "workshop.render_interface.dx12/dx12_render_command_queue.h"
#include "workshop.render_interface.dx12/dx12_render_descriptor_heap.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.windowing/window.h"

#include <algorithm>

namespace ws {

dx12_render_interface::~dx12_render_interface() = default;

dx12_render_interface::dx12_render_interface()
{
}

void dx12_render_interface::register_init(init_list& list)
{
    list.add_step(
        "Create DX12 Device",
        [this]() -> result<void> { return create_device(); },
        [this]() -> result<void> { return destroy_device(); }
    );
    list.add_step(
        "Create DX12 Command Queues",
        [this]() -> result<void> { return create_command_queues(); },
        [this]() -> result<void> { return destroy_command_queues(); }
    );
    list.add_step(
        "Create DX12 Heaps",
        [this]() -> result<void> { return create_heaps(); },
        [this]() -> result<void> { return destroy_heaps(); }
    );
}

std::unique_ptr<render_swapchain> dx12_render_interface::create_swapchain(window& for_window, const char* debug_name)
{
    std::unique_ptr<dx12_render_swapchain> swap = std::make_unique<dx12_render_swapchain>(*this, for_window, debug_name);
    if (!swap->create_resources())
    {
        return nullptr;
    }

    return swap;
}

std::unique_ptr<render_fence> dx12_render_interface::create_fence(const char* debug_name)
{
    std::unique_ptr<dx12_render_fence> instance = std::make_unique<dx12_render_fence>(*this, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

bool dx12_render_interface::is_tearing_allowed()
{
    return m_allow_tearing;
}

Microsoft::WRL::ComPtr<IDXGIFactory4> dx12_render_interface::get_dxgi_factory()
{
    return m_dxgi_factory;
}

render_command_queue& dx12_render_interface::get_graphics_queue()
{
    return *m_graphics_queue;
}

Microsoft::WRL::ComPtr<ID3D12Device> dx12_render_interface::get_device()
{
    return m_device;
}

dx12_render_descriptor_heap& dx12_render_interface::get_uav_descriptor_heap()
{
    return *m_uav_descriptor_heap;
}

dx12_render_descriptor_heap& dx12_render_interface::get_sampler_descriptor_heap()
{
    return *m_sampler_descriptor_heap;
}

dx12_render_descriptor_heap& dx12_render_interface::get_rtv_descriptor_heap()
{
    return *m_rtv_descriptor_heap;
}

dx12_render_descriptor_heap& dx12_render_interface::get_dsv_descriptor_heap()
{
    return *m_dsv_descriptor_heap;
}

size_t dx12_render_interface::get_pipeline_depth()
{
    return k_max_pipeline_depth;
}

size_t dx12_render_interface::get_frame_index()
{
    return m_frame_index;
}

result<void> dx12_render_interface::create_device()
{
    HRESULT hr = 0;

#ifdef WS_DEBUG
    
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug_interface));
    if (FAILED(hr))
    {
        db_error(render_interface, "D3D12GetDebugInterface failed with error 0x%08x.", hr);
        return false;
    }

    m_debug_interface->EnableDebugLayer();

#endif

    UINT createFactoryFlags = 0;
#ifdef WS_DEBUG
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&m_dxgi_factory));
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateDXGIFactory2 failed with error 0x%08x.", hr);
        return false;
    }

    if (result<void> ret = select_adapter(); !ret)
    {
        return ret;
    }

    hr = D3D12CreateDevice(m_dxgi_adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), (void**)&m_device);
    if (FAILED(hr))
    {
        db_error(render_interface, "D3D12CreateDevice failed with error 0x%08x.", hr);
        return false;
    }

    if (result<void> ret = check_feature_support(); !ret)
    {
        return ret;
    }

#ifdef WS_DEBUG
    hr = m_device.As(&m_info_queue);
    if (FAILED(hr))
    {
        db_error(render_interface, "Failed to get D2D12InfoQueue from device with error 0x%08x.", hr);
        return false;
    }

    m_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    m_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    m_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
#endif

    return true;
}

result<void> dx12_render_interface::destroy_device()
{
    m_device = nullptr;
    m_dxgi_factory = nullptr;
    m_dxgi_adapter = nullptr;
    m_debug_interface = nullptr;

    return true;
}

result<void> dx12_render_interface::check_feature_support()
{
    HRESULT hr = 0;

    hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, reinterpret_cast<void*>(&m_options), sizeof(m_options));
    if (FAILED(hr))
    {
        db_error(render_interface, "CheckFeatureSupport failed with error 0x%08x.", hr);
        return false;
    }

    hr = m_dxgi_factory.As(&m_dxgi_factory_5);
    if (FAILED(hr))
    {
        db_warning(render_interface, "Failed to get IDXGIFactory5 with error 0x%08x, assuming no VRR.", hr);
    }
    else
    {
        // This extra var is required due to the size-difference between BOOL and bool ...
        BOOL tearingAllowed = FALSE;

        hr = m_dxgi_factory_5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, reinterpret_cast<void*>(&tearingAllowed), sizeof(tearingAllowed));
        if (FAILED(hr))
        {
            db_error(render_interface, "CheckFeatureSupport failed with error 0x%08x.", hr);
            return false;
        }

        m_allow_tearing = (tearingAllowed == TRUE);
    }

    return true;
}

result<void> dx12_render_interface::select_adapter()
{
    using DescriptionPair = std::pair<DXGI_ADAPTER_DESC1, Microsoft::WRL::ComPtr<IDXGIAdapter1>>;

    HRESULT hr;
    std::vector<DescriptionPair> descriptions;

    for (UINT i = 0; true; i++)
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

        if (m_dxgi_factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND)
        {
            break;
        }

        DXGI_ADAPTER_DESC1 adapterDescription;
        hr = adapter->GetDesc1(&adapterDescription);
        if (FAILED(hr))
        {
            db_error(render_interface, "IDXGIAdapter1::GetDesc1 failed with error 0x%08x.", hr);
            return false;
        }

        descriptions.push_back({ adapterDescription, adapter });
    }

    if (descriptions.empty())
    {
        db_error(render_interface, "Failed to get any valid graphics adapters.");
        return false;
    }

    auto ScoreDescription = [](const DXGI_ADAPTER_DESC1& desc)
    {
        int64_t score = 0;

        // Massively deprioritise software adapters.
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            score -= 1'000'000'000;
        }

        // Prioritise adapters with dedicated video memory.
        score += desc.DedicatedVideoMemory * 1'000'000;

        // If no dedicated video memory, then prioritise by most system memory.
        score += (desc.SharedSystemMemory + desc.DedicatedSystemMemory);

        return score;
    };

    auto Comparator = [&ScoreDescription](const DescriptionPair& a, const DescriptionPair& b)
    {
        return ScoreDescription(a.first) > ScoreDescription(b.first);
    };

    std::sort(descriptions.begin(), descriptions.end(), Comparator);

    db_log(render_interface, "Graphics Adapters:");
    for (size_t i = 0; i < descriptions.size(); i++)
    {
        const DXGI_ADAPTER_DESC1& description = descriptions[i].first;

        std::string name = narrow_string(description.Description);

        db_log(render_interface, "[%c] %-40s", i == 0 ? '*' : ' ', name.c_str());
        db_log(render_interface, "     VendorId:              0x%04x", description.VendorId);
        db_log(render_interface, "     DeviceId:              0x%04x", description.DeviceId);
        db_log(render_interface, "     DedicatedVideoMemory:  %zi mb", description.DedicatedVideoMemory / 1024 / 1024);
        db_log(render_interface, "     SharedSystemMemory:    %zi mb", description.SharedSystemMemory / 1024 / 1024);
        db_log(render_interface, "     DedicatedSystemMemory: %zi mb", description.DedicatedSystemMemory / 1024 / 1024);
    }

    hr = descriptions[0].second.As(&m_dxgi_adapter);
    if (FAILED(hr))
    {
        db_error(render_interface, "Failed to cast dxgi adapter with error 0x%08x.", hr);
        return false;
    }

    return true;
}

result<void> dx12_render_interface::create_command_queues()
{
    m_graphics_queue = std::make_unique<dx12_render_command_queue>(*this, "Graphics Command Queue", D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (!m_graphics_queue->create_resources())
    {
        return false;
    }

    return true;
}

result<void> dx12_render_interface::destroy_command_queues()
{
    m_graphics_queue = nullptr;
    return true;
}

result<void> dx12_render_interface::create_heaps()
{
    m_uav_descriptor_heap = std::make_unique<dx12_render_descriptor_heap>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100'000);
    if (!m_uav_descriptor_heap->create_resources())
    {
        return false;
    }

    m_sampler_descriptor_heap = std::make_unique<dx12_render_descriptor_heap>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 100'000);
    if (!m_sampler_descriptor_heap->create_resources())
    {
        return false;
    }

    m_rtv_descriptor_heap = std::make_unique<dx12_render_descriptor_heap>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1'000);
    if (!m_rtv_descriptor_heap->create_resources())
    {
        return false;
    }

    m_dsv_descriptor_heap = std::make_unique<dx12_render_descriptor_heap>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1'000);
    if (!m_dsv_descriptor_heap->create_resources())
    {
        return false;
    }

    return true;
}

result<void> dx12_render_interface::destroy_heaps()
{
    m_uav_descriptor_heap = nullptr;
    m_sampler_descriptor_heap = nullptr;
    m_rtv_descriptor_heap = nullptr;
    m_dsv_descriptor_heap = nullptr;

    return true;
}

void dx12_render_interface::new_frame()
{
    m_frame_index++;
}

}; // namespace ws
