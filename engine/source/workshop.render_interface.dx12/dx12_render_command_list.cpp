// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_render_command_list.h"
#include "workshop.render_interface.dx12/dx12_render_command_queue.h"
#include "workshop.render_interface.dx12/dx12_render_interface.h"
#include "workshop.render_interface.dx12/dx12_render_target.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.core/drawing/color.h"
#include "workshop.windowing/window.h"

namespace ws {

dx12_render_command_list::dx12_render_command_list(dx12_render_interface& renderer, const char* debug_name, dx12_render_command_queue& queue)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_queue(queue)
{
}

dx12_render_command_list::~dx12_render_command_list()
{
    m_command_list = nullptr;
}

result<void> dx12_render_command_list::create_resources()
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

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> dx12_render_command_list::get_dx_command_list()
{
    return m_command_list;
}

bool dx12_render_command_list::is_open()
{
    return m_opened;
}

void dx12_render_command_list::set_allocated_frame(size_t frame)
{
    m_allocated_frame_index = frame;
}

void dx12_render_command_list::open()
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

void dx12_render_command_list::close()
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

void dx12_render_command_list::barrier(render_target& resource, render_resource_state source_state, render_resource_state destination_state)
{
    dx12_render_target& dx12_resource = static_cast<dx12_render_target&>(resource);

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = dx12_resource.get_buffer().Get();
    barrier.Transition.StateBefore = to_dx12_resource_state(source_state);
    barrier.Transition.StateAfter = to_dx12_resource_state(destination_state);
    barrier.Transition.Subresource = 0;
    m_command_list->ResourceBarrier(1, &barrier);
}

void dx12_render_command_list::clear(render_target& resource, const color& destination)
{
    dx12_render_target& dx12_resource = static_cast<dx12_render_target&>(resource);

    FLOAT color[] = { destination.r, destination.g, destination.b, destination.a };
    m_command_list->ClearRenderTargetView(dx12_resource.get_rtv(), color, 0, nullptr);
}

}; // namespace ws
