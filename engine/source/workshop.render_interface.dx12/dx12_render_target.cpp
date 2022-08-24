// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_render_target.h"
#include "workshop.render_interface.dx12/dx12_render_interface.h"
#include "workshop.render_interface.dx12/dx12_render_command_queue.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.windowing/window.h"

namespace ws {

dx12_render_target::dx12_render_target(dx12_render_interface& renderer, const char* debug_name, Microsoft::WRL::ComPtr<ID3D12Resource> buffer)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_buffer(buffer)
{
}

dx12_render_target::~dx12_render_target()
{
    m_buffer = nullptr;

    if (m_rtv.ptr != 0)
    {
        m_renderer.get_rtv_descriptor_heap().free(m_rtv);
    }
}

result<void> dx12_render_target::create_resources()
{
    m_buffer->SetName(widen_string(m_debug_name).c_str());

    m_rtv = m_renderer.get_rtv_descriptor_heap().allocate();
    m_renderer.get_device()->CreateRenderTargetView(m_buffer.Get(), nullptr, m_rtv);

    return true;
}

Microsoft::WRL::ComPtr<ID3D12Resource> dx12_render_target::get_buffer()
{
    return m_buffer;
}

D3D12_CPU_DESCRIPTOR_HANDLE dx12_render_target::get_rtv()
{
    return m_rtv;
}

}; // namespace ws
