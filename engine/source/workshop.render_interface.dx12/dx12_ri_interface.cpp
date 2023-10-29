// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_swapchain.h"
#include "workshop.render_interface.dx12/dx12_ri_fence.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_heap.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include "workshop.render_interface.dx12/dx12_ri_shader_compiler.h"
#include "workshop.render_interface.dx12/dx12_ri_texture_compiler.h"
#include "workshop.render_interface.dx12/dx12_ri_pipeline.h"
#include "workshop.render_interface.dx12/dx12_ri_param_block_archetype.h"
#include "workshop.render_interface.dx12/dx12_ri_texture.h"
#include "workshop.render_interface.dx12/dx12_ri_sampler.h"
#include "workshop.render_interface.dx12/dx12_ri_upload_manager.h"
#include "workshop.render_interface.dx12/dx12_ri_query_manager.h"
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"
#include "workshop.render_interface.dx12/dx12_ri_layout_factory.h"
#include "workshop.render_interface.dx12/dx12_ri_query.h"
#include "workshop.render_interface.dx12/dx12_ri_small_buffer_allocator.h"
#include "workshop.render_interface.dx12/dx12_ri_raytracing_tlas.h"
#include "workshop.render_interface.dx12/dx12_ri_raytracing_blas.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"
#include "workshop.core/filesystem/file.h"

#include <algorithm>

namespace ws {

dx12_render_interface::~dx12_render_interface() = default;

dx12_render_interface::dx12_render_interface(size_t ray_type_count, size_t ray_domain_count)
    : m_ray_type_count(ray_type_count)
    , m_ray_domain_count(ray_domain_count)
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
    list.add_step(
        "Create DX12 Misc",
        [this]() -> result<void> { return create_misc(); },
        [this]() -> result<void> { return destroy_misc(); }
    );
}

std::unique_ptr<ri_swapchain> dx12_render_interface::create_swapchain(window& for_window, const char* debug_name)
{
    std::unique_ptr<dx12_ri_swapchain> swap = std::make_unique<dx12_ri_swapchain>(*this, for_window, debug_name);
    if (!swap->create_resources())
    {
        return nullptr;
    }

    return swap;
}

std::unique_ptr<ri_fence> dx12_render_interface::create_fence(const char* debug_name)
{
    std::unique_ptr<dx12_ri_fence> instance = std::make_unique<dx12_ri_fence>(*this, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_shader_compiler> dx12_render_interface::create_shader_compiler()
{
    std::unique_ptr<dx12_ri_shader_compiler> instance = std::make_unique<dx12_ri_shader_compiler>(*this);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_texture_compiler> dx12_render_interface::create_texture_compiler()
{
    std::unique_ptr<dx12_ri_texture_compiler> instance = std::make_unique<dx12_ri_texture_compiler>(*this);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_pipeline> dx12_render_interface::create_pipeline(const ri_pipeline::create_params& params, const char* debug_name)
{
    std::unique_ptr<dx12_ri_pipeline> instance = std::make_unique<dx12_ri_pipeline>(*this, params, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_param_block_archetype> dx12_render_interface::create_param_block_archetype(const ri_param_block_archetype::create_params& params, const char* debug_name)
{
    std::unique_ptr<dx12_ri_param_block_archetype> instance = std::make_unique<dx12_ri_param_block_archetype>(*this, params, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_texture> dx12_render_interface::create_texture(const ri_texture::create_params& params, const char* debug_name)
{
    std::unique_ptr<dx12_ri_texture> instance = std::make_unique<dx12_ri_texture>(*this, debug_name, params);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_sampler> dx12_render_interface::create_sampler(const ri_sampler::create_params& params, const char* debug_name)
{
    std::unique_ptr<dx12_ri_sampler> instance = std::make_unique<dx12_ri_sampler>(*this, debug_name, params);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_buffer> dx12_render_interface::create_buffer(const ri_buffer::create_params& params, const char* debug_name)
{
    std::unique_ptr<dx12_ri_buffer> instance = std::make_unique<dx12_ri_buffer>(*this, debug_name, params);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_layout_factory> dx12_render_interface::create_layout_factory(ri_data_layout layout, ri_layout_usage usage)
{
    return std::make_unique<dx12_ri_layout_factory>(*this, layout, usage);
}

std::unique_ptr<ri_query> dx12_render_interface::create_query(const ri_query::create_params& params, const char* debug_name)
{
    std::unique_ptr<dx12_ri_query> instance = std::make_unique<dx12_ri_query>(*this, debug_name, params);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_raytracing_blas> dx12_render_interface::create_raytracing_blas(const char* debug_name)
{
    std::unique_ptr<dx12_ri_raytracing_blas> instance = std::make_unique<dx12_ri_raytracing_blas>(*this, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_raytracing_tlas> dx12_render_interface::create_raytracing_tlas(const char* debug_name)
{
    std::unique_ptr<dx12_ri_raytracing_tlas> instance = std::make_unique<dx12_ri_raytracing_tlas>(*this, debug_name);
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

ri_command_queue& dx12_render_interface::get_graphics_queue()
{
    return *m_graphics_queue;
}

ri_command_queue& dx12_render_interface::get_copy_queue()
{
    return *m_copy_queue;
}

Microsoft::WRL::ComPtr<ID3D12Device5> dx12_render_interface::get_device()
{
    return m_device_5;
}

dx12_ri_descriptor_heap& dx12_render_interface::get_srv_descriptor_heap()
{
    return *m_srv_descriptor_heap;
}

dx12_ri_descriptor_heap& dx12_render_interface::get_sampler_descriptor_heap()
{
    return *m_sampler_descriptor_heap;
}

dx12_ri_descriptor_heap& dx12_render_interface::get_rtv_descriptor_heap()
{
    return *m_rtv_descriptor_heap;
}

dx12_ri_descriptor_heap& dx12_render_interface::get_dsv_descriptor_heap()
{
    return *m_dsv_descriptor_heap;
}

dx12_ri_small_buffer_allocator& dx12_render_interface::get_small_buffer_allocator()
{
    return *m_small_buffer_allocator;
}

dx12_ri_descriptor_table& dx12_render_interface::get_descriptor_table(ri_descriptor_table table)
{
    return *m_descriptor_tables[static_cast<size_t>(table)];
}

size_t dx12_render_interface::get_pipeline_depth()
{
    return k_max_pipeline_depth;
}

dx12_ri_upload_manager& dx12_render_interface::get_upload_manager()
{
    return *m_upload_manager;
}

dx12_ri_query_manager& dx12_render_interface::get_query_manager()
{
    return *m_query_manager;
}

size_t dx12_render_interface::get_frame_index()
{
    return m_frame_index;
}

size_t dx12_render_interface::get_ray_domain_count()
{
    return m_ray_domain_count;
}

size_t dx12_render_interface::get_ray_type_count()
{
    return m_ray_type_count;
}

result<void> dx12_render_interface::create_device()
{
    HRESULT hr = 0;

    bool should_debug = false;
#ifdef WS_DEBUG
    should_debug = true;
#endif
    if (ws::is_option_set("directx_debug"))
    {
        should_debug = true;
    }

    if (should_debug)
    {
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug_interface));
        if (FAILED(hr))
        {
            db_error(render_interface, "D3D12GetDebugInterface failed with error 0x%08x.", hr);
            return false;
        }

        m_debug_interface->EnableDebugLayer();

#if 0
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_dread_interface));
        if (FAILED(hr))
        {
            db_error(render_interface, "D3D12GetDebugInterface failed with error 0x%08x.", hr);
            return false;
        }

        m_dread_interface->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        m_dread_interface->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
#endif
    }

    UINT createFactoryFlags = 0;
    if (should_debug)
    {
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
    }

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

    hr = m_device.As(&m_device_5);
    if (FAILED(hr))
    {
        db_error(render_interface, "Failed to get ID3D12Device5 from device with error 0x%08x.", hr);
        return false;
    }

    if (result<void> ret = check_feature_support(); !ret)
    {
        return ret;
    }

    if (should_debug)
    {
        hr = m_device.As(&m_info_queue);
        if (FAILED(hr))
        {
            db_error(render_interface, "Failed to get D2D12InfoQueue from device with error 0x%08x.", hr);
            return false;
        }

        m_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        m_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        m_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
    }

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

    hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, reinterpret_cast<void*>(&m_options_5), sizeof(m_options_5));
    if (FAILED(hr))
    {
        db_error(render_interface, "CheckFeatureSupport failed with error 0x%08x. GPU likely does not supported the required features.", hr);
        return false;
    }

    hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, reinterpret_cast<void*>(&m_options), sizeof(m_options));
    if (FAILED(hr))
    {
        db_error(render_interface, "CheckFeatureSupport failed with error 0x%08x. GPU likely does not supported the required features.", hr);
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

    if (m_options_5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
    {
        db_error(render_interface, "Required ray tracing not supported on this gpu.", hr);
        return false;
    }

    if (m_options.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
    {
        db_error(render_interface, "Required resource binding tier not supported on this gpu.", hr);
        return false;
    }

    if (m_options.TiledResourcesTier < D3D12_TILED_RESOURCES_TIER_3)
    {
        db_error(render_interface, "Required tiled resource tier not supported on this gpu.", hr);
        return false;
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
    m_graphics_queue = std::make_unique<dx12_ri_command_queue>(*this, "Graphics Command Queue", D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (!m_graphics_queue->create_resources())
    {
        return false;
    }

    m_copy_queue = std::make_unique<dx12_ri_command_queue>(*this, "Copy Command Queue", D3D12_COMMAND_LIST_TYPE_COPY);
    if (!m_copy_queue->create_resources())
    {
        return false;
    }

    return true;
}

result<void> dx12_render_interface::destroy_command_queues()
{
    m_copy_queue = nullptr;
    m_graphics_queue = nullptr;
    return true;
}

result<void> dx12_render_interface::create_heaps()
{
    size_t srv_heap_size = 0;
    size_t sampler_heap_size = 0;
    size_t rtv_heap_size = 0;
    size_t dsv_heap_size = 0;

    dsv_heap_size += k_descriptor_table_sizes[(int)ri_descriptor_table::depth_stencil];
    rtv_heap_size += k_descriptor_table_sizes[(int)ri_descriptor_table::render_target];
    sampler_heap_size += k_descriptor_table_sizes[(int)ri_descriptor_table::sampler];

    for (size_t i = 0; i < k_descriptor_table_sizes.size(); i++)
    {
        if (i != (int)ri_descriptor_table::depth_stencil &&
            i != (int)ri_descriptor_table::render_target &&
            i != (int)ri_descriptor_table::sampler)
        {
            srv_heap_size += k_descriptor_table_sizes[i];
        }
    }

    m_srv_descriptor_heap = std::make_unique<dx12_ri_descriptor_heap>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srv_heap_size);
    if (!m_srv_descriptor_heap->create_resources())
    {
        return false;
    }

    m_sampler_descriptor_heap = std::make_unique<dx12_ri_descriptor_heap>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, sampler_heap_size);
    if (!m_sampler_descriptor_heap->create_resources())
    {
        return false;
    }

    m_rtv_descriptor_heap = std::make_unique<dx12_ri_descriptor_heap>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtv_heap_size);
    if (!m_rtv_descriptor_heap->create_resources())
    {
        return false;
    }

    m_dsv_descriptor_heap = std::make_unique<dx12_ri_descriptor_heap>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, dsv_heap_size);
    if (!m_dsv_descriptor_heap->create_resources())
    {
        return false;
    }

    // Create tables for each resource type.
    for (size_t i = 0; i < static_cast<size_t>(ri_descriptor_table::COUNT); i++)
    {
        std::unique_ptr<dx12_ri_descriptor_table>& table = m_descriptor_tables[i];
        
        table = std::make_unique<dx12_ri_descriptor_table>(*this, static_cast<ri_descriptor_table>(i));
        if (!table->create_resources())
        {
            return false;
        }
    }

    return true;
}

result<void> dx12_render_interface::destroy_heaps()
{
    m_srv_descriptor_heap = nullptr;
    m_sampler_descriptor_heap = nullptr;
    m_rtv_descriptor_heap = nullptr;
    m_dsv_descriptor_heap = nullptr;

    return true;
}


result<void> dx12_render_interface::create_misc()
{
    m_upload_manager = std::make_unique<dx12_ri_upload_manager>(*this);
    if (!m_upload_manager->create_resources())
    {
        return false;
    }

    m_query_manager = std::make_unique<dx12_ri_query_manager>(*this, k_maximum_queries);
    if (!m_query_manager->create_resources())
    {
        return false;
    }

    m_small_buffer_allocator = std::make_unique<dx12_ri_small_buffer_allocator>(*this);
    if (!m_small_buffer_allocator->create_resources())
    {
        return false;
    }

    return true;
}

result<void> dx12_render_interface::destroy_misc()
{
    m_query_manager = nullptr;
    m_upload_manager = nullptr;

    return true;
}

void dx12_render_interface::begin_frame()
{
    m_frame_index++;

    process_pending_deletes();

    m_graphics_queue->begin_frame();
    m_copy_queue->begin_frame();
    m_query_manager->begin_frame();

    process_as_build_requests();
}

void dx12_render_interface::end_frame()
{
    m_graphics_queue->end_frame();
    m_copy_queue->end_frame();
}

void dx12_render_interface::flush_uploads()
{
    profile_marker(profile_colors::render, "flush uploads");

    m_upload_manager->new_frame(m_frame_index);
}

void dx12_render_interface::queue_as_build(dx12_ri_raytracing_tlas* tlas)
{
    std::scoped_lock lock(m_pending_as_build_mutex);
    m_pending_tlas_builds.insert(tlas);
}

void dx12_render_interface::queue_as_build(dx12_ri_raytracing_blas* blas)
{
    std::scoped_lock lock(m_pending_as_build_mutex);
    m_pending_blas_builds.insert(blas);
}

void dx12_render_interface::process_as_build_requests()
{
    std::scoped_lock lock(m_pending_as_build_mutex);

    if (m_pending_blas_builds.empty() && m_pending_tlas_builds.empty())
    {
        return;
    }

    dx12_ri_command_list& build_list = static_cast<dx12_ri_command_list&>(m_graphics_queue->alloc_command_list());
    build_list.open();

    // Order is important, blas should be built before tlas.
    db_log(renderer, "Building raytracing AS: Bottom=%zi Top=%zi", m_pending_blas_builds.size(), m_pending_tlas_builds.size());

    for (dx12_ri_raytracing_blas* blas : m_pending_blas_builds)
    {
        blas->build(build_list);
    }

    for (dx12_ri_raytracing_tlas* tlas : m_pending_tlas_builds)
    {
        tlas->build(build_list);
    }

    build_list.close();

    m_pending_tlas_builds.clear();
    m_pending_blas_builds.clear();

    // We flush uploads here before dispatching the builds as we will likely
    // have updated various tlas/blas buffers that we want to be reflected on the gpu
    // when the build occurs.
    flush_uploads();

    profile_gpu_marker(*m_graphics_queue, profile_colors::gpu_view, "build raytracing structures");
    m_graphics_queue->execute(build_list);
}

void dx12_render_interface::drain_deferred()
{
    std::scoped_lock lock(m_pending_deletion_mutex);

    for (size_t i = 0; i < k_max_pipeline_depth; i++)
    {
        auto& queue = m_pending_deletions[i];
        for (deferred_delete_function_t& functor : queue)
        {
            functor();
        }
        queue.clear();
    }
}

void dx12_render_interface::process_pending_deletes()
{
    std::scoped_lock lock(m_pending_deletion_mutex);

    profile_marker(profile_colors::render, "process pending deletes");

    size_t queue_index = (m_frame_index % k_max_pipeline_depth);
    auto& queue = m_pending_deletions[queue_index];
    for (deferred_delete_function_t& functor : queue)
    {
        functor();
    }
    queue.clear();
}

void dx12_render_interface::defer_delete(const deferred_delete_function_t& func)
{
    std::scoped_lock lock(m_pending_deletion_mutex);

    size_t queue_index = (m_frame_index % k_max_pipeline_depth);
    m_pending_deletions[queue_index].push_back(func);
}

void dx12_render_interface::get_vram_usage(size_t& out_local, size_t& out_non_local)
{
    DXGI_QUERY_VIDEO_MEMORY_INFO local_video_memory_info;
    DXGI_QUERY_VIDEO_MEMORY_INFO non_local_video_memory_info;
    m_dxgi_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local_video_memory_info);
    m_dxgi_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &non_local_video_memory_info);

    out_local = local_video_memory_info.CurrentUsage;
    out_non_local = non_local_video_memory_info.CurrentUsage;
}

size_t dx12_render_interface::get_cube_map_face_index(ri_cube_map_face face)
{
    std::array<size_t, 6> lookup = {
        0, // x_pos
        1, // x_neg
        2, // y_pos
        3, // y_neg
        4, // z_pos
        5, // z_neg
    };

    return lookup[static_cast<size_t>(face)];
}

}; // namespace ws
