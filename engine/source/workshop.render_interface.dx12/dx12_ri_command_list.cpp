// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_command_list.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_texture.h"
#include "workshop.render_interface.dx12/dx12_ri_pipeline.h"
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"
#include "workshop.render_interface.dx12/dx12_ri_param_block.h"
#include "workshop.render_interface.dx12/dx12_ri_param_block_archetype.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include "workshop.render_interface.dx12/dx12_ri_query.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.core/drawing/color.h"
#include "workshop.window_interface/window.h"

#include "thirdparty/pix/include/pix3.h"

namespace ws {

dx12_ri_command_list::dx12_ri_command_list(dx12_render_interface& renderer, const char* debug_name, dx12_ri_command_queue& queue)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_queue(queue)
{
}

dx12_ri_command_list::~dx12_ri_command_list()
{
    m_command_list = nullptr;
}

result<void> dx12_ri_command_list::create_resources()
{
    HRESULT hr = m_renderer.get_device()->CreateCommandList(
        0, 
        m_queue.get_dx_queue_type(), 
        m_queue.get_current_command_allocator().Get(), 
        nullptr, 
        IID_PPV_ARGS(&m_command_list));

    if (FAILED(hr))
    {
        db_error(render_interface, "CreateCommandList failed with error 0x%08x.", hr);
        return false;
    }

    hr = m_command_list->Close();
    if (FAILED(hr))
    {
        db_error(render_interface, "CommandList Reset failed with error 0x%08x.", hr);
        return false;
    }

    return true;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> dx12_ri_command_list::get_dx_command_list()
{
    return m_command_list;
}

bool dx12_ri_command_list::is_open()
{
    return m_opened;
}

void dx12_ri_command_list::set_allocated_frame(size_t frame)
{
    m_allocated_frame_index = frame;
}

void dx12_ri_command_list::open()
{
    db_assert(!m_opened);
    db_assert_message(m_renderer.get_frame_index() == m_allocated_frame_index, "Command list is only valid for the frame its allocated on.");

    HRESULT hr = m_command_list->Reset(m_queue.get_current_command_allocator().Get(), nullptr);
    if (FAILED(hr))
    {
        db_error(render_interface, "CommandList Reset failed with error 0x%08x.", hr);
    }

    m_opened = true;
}

void dx12_ri_command_list::close()
{
    db_assert(m_opened);
    db_assert_message(m_renderer.get_frame_index() == m_allocated_frame_index, "Command list is only valid for the frame its allocated on.");

    HRESULT hr = m_command_list->Close();
    if (FAILED(hr))
    {
        db_error(render_interface, "CommandList Close failed with error 0x%08x.", hr);
    }

    m_opened = false;
}

void dx12_ri_command_list::barrier(ri_texture& resource, ri_resource_state source_state, ri_resource_state destination_state)
{
    dx12_ri_texture& dx12_resource = static_cast<dx12_ri_texture&>(resource);
    barrier(dx12_resource.get_resource(), dx12_resource.get_initial_state(), source_state, destination_state);
}

void dx12_ri_command_list::barrier(ri_buffer& resource, ri_resource_state source_state, ri_resource_state destination_state)
{
    dx12_ri_buffer& dx12_resource = static_cast<dx12_ri_buffer&>(resource);
    barrier(dx12_resource.get_resource(), dx12_resource.get_initial_state(), source_state, destination_state);
}

void dx12_ri_command_list::barrier(ID3D12Resource* resource, ri_resource_state resource_initial_state, ri_resource_state source_state, ri_resource_state destination_state)
{
    if (source_state == ri_resource_state::initial)
    {
        source_state = resource_initial_state;
    }
    if (destination_state == ri_resource_state::initial)
    {
        destination_state = resource_initial_state;
    }

    if (source_state == destination_state)
    {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = ri_to_dx12(source_state);
    barrier.Transition.StateAfter = ri_to_dx12(destination_state);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_command_list->ResourceBarrier(1, &barrier);

    //db_log(core, "barrier: %p : %i -> %i", resource, (int)source_state, (int)destination_state);
}

void dx12_ri_command_list::clear(ri_texture_view resource, const color& destination)
{
    dx12_ri_texture& dx12_resource = static_cast<dx12_ri_texture&>(*resource.texture);

    size_t mip = resource.mip;
    size_t slice = resource.slice;
    if (mip == ri_texture_view::k_unset)
    {
        mip = 0;
    }
    if (slice == ri_texture_view::k_unset)
    {
        slice = 0;
    }

    FLOAT color[] = { destination.r, destination.g, destination.b, destination.a };
    m_command_list->ClearRenderTargetView(dx12_resource.get_rtv(slice, mip).cpu_handle, color, 0, nullptr);
}

void dx12_ri_command_list::clear_depth(ri_texture_view resource, float depth, size_t stencil)
{
    db_assert(resource.mip == 0 || resource.mip == ri_texture_view::k_unset);

    dx12_ri_texture& dx12_resource = static_cast<dx12_ri_texture&>(*resource.texture);

    size_t slice = resource.slice;
    if (slice == ri_texture_view::k_unset)
    {
        slice = 0;
    }

    m_command_list->ClearDepthStencilView(dx12_resource.get_dsv(slice).cpu_handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, static_cast<UINT8>(stencil), 0, nullptr);
}

void dx12_ri_command_list::set_pipeline(ri_pipeline& pipeline)
{
    dx12_ri_pipeline& dx12_pipeline = static_cast<dx12_ri_pipeline&>(pipeline);
    m_active_pipeline = &dx12_pipeline;

    if (dx12_pipeline.is_compute())
    {
        m_command_list->SetComputeRootSignature(dx12_pipeline.get_root_signature());
    }
    else
    {
        m_command_list->SetGraphicsRootSignature(dx12_pipeline.get_root_signature());
    }

    m_command_list->SetPipelineState(dx12_pipeline.get_pipeline_state());

    std::array<ID3D12DescriptorHeap*, 2> heaps = {
        m_renderer.get_sampler_descriptor_heap().get_resource(),
        m_renderer.get_srv_descriptor_heap().get_resource()
    };
    m_command_list->SetDescriptorHeaps(2, heaps.data());

    UINT table_index = 0;

    // Bind all the bindless descriptor tables.
    ri_pipeline::create_params create_params = pipeline.get_create_params();
    for (size_t i = 0; i < create_params.descriptor_tables.size(); i++)
    {
        ri_descriptor_table& table = create_params.descriptor_tables[i];        

        dx12_ri_descriptor_table& descriptor_table = m_renderer.get_descriptor_table(table);

        if (dx12_pipeline.is_compute())
        {
            m_command_list->SetComputeRootDescriptorTable(table_index++, descriptor_table.get_base_allocation().gpu_handle);
        }
        else
        {
            m_command_list->SetGraphicsRootDescriptorTable(table_index++, descriptor_table.get_base_allocation().gpu_handle);
        }
    }
}

void dx12_ri_command_list::set_param_blocks(const std::vector<ri_param_block*> param_blocks)
{
    db_assert(m_active_pipeline != nullptr);

    const auto& archetype_list = m_active_pipeline->get_create_params().param_block_archetypes;

    // Param blocks immediately follow the descriptor tables in the root sig.
    size_t base_param_block_root_parameter = m_active_pipeline->get_create_params().descriptor_tables.size();

    size_t cbv_index = 0;
    for (size_t i = 0; i < archetype_list.size(); i++)
    {
        ri_param_block_archetype* archetype = archetype_list[i];

        // Instance param blocks are passed in via the instant buffer. We don't need to provide them here.
        if (archetype->get_create_params().scope == ri_data_scope::instance)
        {
            continue;
        }

        // Find appropriate param block in input.
        bool found = false;

        for (ri_param_block* base : param_blocks)
        {
            dx12_ri_param_block* input = static_cast<dx12_ri_param_block*>(base);
            if (input->get_archetype() == archetype)
            {
                if (m_active_pipeline->is_compute())
                {
                    m_command_list->SetComputeRootConstantBufferView(
                        static_cast<UINT>(base_param_block_root_parameter + cbv_index),
                        reinterpret_cast<UINT64>(input->consume())
                    );
                }
                else
                {
                    m_command_list->SetGraphicsRootConstantBufferView(
                        static_cast<UINT>(base_param_block_root_parameter + cbv_index),
                        reinterpret_cast<UINT64>(input->consume())
                    );
                }

                cbv_index++;
                found = true;
                break;
            }
        }

        if (!found)
        {
            db_error(renderer, "set_param_blocks didn't include param block expected by pipeline '%s'.", archetype->get_name());
            return;
        }
    }
}

void dx12_ri_command_list::set_viewport(const recti& rect)
{
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = static_cast<float>(rect.x);
    viewport.TopLeftY = static_cast<float>(rect.y);
    viewport.Width = static_cast<float>(rect.width);
    viewport.Height = static_cast<float>(rect.height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    m_command_list->RSSetViewports(1, &viewport);
}

void dx12_ri_command_list::set_scissor(const recti& rect)
{    
    D3D12_RECT scissor;
    scissor.left = rect.x;
    scissor.top = rect.y;
    scissor.right = rect.x + rect.width;
    scissor.bottom = rect.y + rect.height;

    m_command_list->RSSetScissorRects(1, &scissor);
}

void dx12_ri_command_list::set_blend_factor(const vector4& factor)
{
    FLOAT blend_factor[4];
    blend_factor[0] = factor.x;
    blend_factor[1] = factor.y;
    blend_factor[2] = factor.z;
    blend_factor[3] = factor.w;
    m_command_list->OMSetBlendFactor(blend_factor);
}

void dx12_ri_command_list::set_stencil_ref(uint32_t value)
{
    m_command_list->OMSetStencilRef(value);
}

void dx12_ri_command_list::set_primitive_topology(ri_primitive value)
{
    m_command_list->IASetPrimitiveTopology(ri_to_dx12(value));
}

void dx12_ri_command_list::set_index_buffer(ri_buffer& buffer)
{
    dx12_ri_buffer* dx12_buffer = static_cast<dx12_ri_buffer*>(&buffer);

    DXGI_FORMAT format;
    size_t element_size = dx12_buffer->get_element_size();
    if (element_size == 2)
    {
        format = DXGI_FORMAT_R16_UINT;
    }
    else if (element_size == 4)
    {
        format = DXGI_FORMAT_R32_UINT;
    }
    else
    {
        db_fatal(renderer, "Element size of buffer was invalid for index buffer.");
    }

    D3D12_INDEX_BUFFER_VIEW view;
    view.BufferLocation = dx12_buffer->get_resource()->GetGPUVirtualAddress();
    view.Format = format;
    view.SizeInBytes = static_cast<UINT>(dx12_buffer->get_element_count() * element_size);

    m_command_list->IASetIndexBuffer(&view);
}

void dx12_ri_command_list::set_render_targets(const std::vector<ri_texture_view>& colors, ri_texture_view depth)
{
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> color_handles;
    for (ri_texture_view value : colors)
    {
        dx12_ri_texture* dx12_tex = static_cast<dx12_ri_texture*>(value.texture);

        size_t slice = value.slice;
        size_t mip = value.mip;
        if (slice == ri_texture_view::k_unset)
        {
            slice = 0;
        }
        if (mip == ri_texture_view::k_unset)
        {
            mip = 0;
        }

        color_handles.push_back(dx12_tex->get_rtv(slice, mip).cpu_handle);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE depth_handle = {};
    if (depth.texture != nullptr)
    {
        db_assert(depth.mip == 0 || depth.mip == ri_texture_view::k_unset);

        dx12_ri_texture* dx12_tex = static_cast<dx12_ri_texture*>(depth.texture);

        size_t slice = depth.slice;
        if (slice == ri_texture_view::k_unset)
        {
            slice = 0;
        }

        depth_handle = dx12_tex->get_dsv(slice).cpu_handle;
    }

    m_command_list->OMSetRenderTargets(
        static_cast<UINT>(color_handles.size()),
        color_handles.data(),
        false,
        depth.texture == nullptr ? nullptr : &depth_handle
    );
}

void dx12_ri_command_list::draw(size_t indexes_per_instance, size_t instance_count, size_t start_index_location)
{
    m_command_list->DrawIndexedInstanced(static_cast<UINT>(indexes_per_instance), static_cast<UINT>(instance_count), static_cast<UINT>(start_index_location), 0, 0);
}

void dx12_ri_command_list::dispatch(size_t group_size_x, size_t group_size_y, size_t group_size_z)
{
    m_command_list->Dispatch(static_cast<UINT>(group_size_x), static_cast<UINT>(group_size_y), static_cast<UINT>(group_size_z));
}

void dx12_ri_command_list::begin_event(const color& color, const char* format, ...)
{
    uint8_t r, g, b, a;
    color.get(r, g, b, a);

    char buffer[1024];

    va_list list;
    va_start(list, format);

    int ret = vsnprintf(buffer, sizeof(buffer), format, list);
    int space_required = ret + 1;
    if (ret >= sizeof(buffer))
    {
        return;
    }

    PIXBeginEvent(m_command_list.Get(), PIX_COLOR(r, g, b), "%s", buffer);

    va_end(list);
}

void dx12_ri_command_list::end_event()
{
    PIXEndEvent(m_command_list.Get());
}

void dx12_ri_command_list::begin_query(ri_query* query)
{
    dx12_ri_query* typed_query = static_cast<dx12_ri_query*>(query);
    typed_query->begin(m_command_list.Get());
}

void dx12_ri_command_list::end_query(ri_query* query)
{
    dx12_ri_query* typed_query = static_cast<dx12_ri_query*>(query);
    typed_query->end(m_command_list.Get());
}

}; // namespace ws
