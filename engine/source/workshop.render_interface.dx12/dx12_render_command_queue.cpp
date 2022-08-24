// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_render_command_queue.h"
#include "workshop.render_interface.dx12/dx12_render_command_list.h"
#include "workshop.render_interface.dx12/dx12_render_interface.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.windowing/window.h"

namespace ws {

dx12_render_command_queue::dx12_render_command_queue(dx12_render_interface& renderer, const char* debug_name, D3D12_COMMAND_LIST_TYPE queue_type)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_queue_type(queue_type)
{
}

dx12_render_command_queue::~dx12_render_command_queue()
{
    for (size_t i = 0; i < dx12_render_interface::k_max_pipeline_depth; i++)
    {
        m_command_allocators[i] = nullptr;
    }

    m_queue = nullptr;
}

result<void> dx12_render_command_queue::create_resources()
{
    HRESULT hr;

    D3D12_COMMAND_QUEUE_DESC description = {};
    description.Type = m_queue_type;
    description.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    description.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    description.NodeMask = 0;

    hr = m_renderer.get_device()->CreateCommandQueue(&description, IID_PPV_ARGS(&m_queue));
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateCommandQueue failed with error 0x%08x.", hr);
        return false;
    }

    for (size_t i = 0; i < dx12_render_interface::k_max_pipeline_depth; i++)
    {
        m_renderer.get_device()->CreateCommandAllocator(m_queue_type, IID_PPV_ARGS(&m_command_allocators[i]));
        if (FAILED(hr))
        {
            db_error(render_interface, "CreateCommandAllocator failed with error 0x%08x.", hr);
            return false;
        }
    }

    return true;
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> dx12_render_command_queue::get_queue()
{
    return m_queue;
}

D3D12_COMMAND_LIST_TYPE dx12_render_command_queue::get_dx_queue_type()
{
    return m_queue_type;
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> dx12_render_command_queue::get_current_command_allocator()
{
    // TODO: we need to make this thread-local when multithreading command list generation.
    return m_command_allocators[m_renderer.get_frame_index() % dx12_render_interface::k_max_pipeline_depth];
}

render_command_list& dx12_render_command_queue::alloc_command_list()
{
    std::scoped_lock lock(m_command_list_mutex);

    size_t frame_index = m_renderer.get_frame_index();

    frame_command_lists& command_lists = m_frame_command_lists[frame_index % dx12_render_interface::k_max_pipeline_depth];

    // If we are reusing a previous frames command lists, make all command lists free again
    // and reset the command allocator.
    if (command_lists.last_used_frame_index != frame_index)
    {
        m_command_allocators[frame_index % dx12_render_interface::k_max_pipeline_depth]->Reset();

        for (size_t i = 0; i < command_lists.next_free_index; i++)
        {
            db_assert_message(!m_command_lists[i]->is_open(), "Reusing command list that hasn't been closed. Command lists should only remain open for the duration of the frame they are allocated on.");
        }

        command_lists.next_free_index = 0;
        command_lists.last_used_frame_index = frame_index;
    }

    // Allocate new command list if we have no more available.
    if (command_lists.next_free_index >= command_lists.command_list_indices.size())
    {
        std::string debug_name = string_format("Command List [%i]", m_command_lists.size());

        std::unique_ptr<dx12_render_command_list> list = std::make_unique<dx12_render_command_list>(m_renderer, debug_name.c_str(), *this);
        if (!list->create_resources())
        {
            db_fatal(render_interface, "Failed to create command list resources.");
        }

        m_command_lists.push_back(std::move(list));
        command_lists.command_list_indices.push_back(m_command_lists.size() - 1);
    }

    // Return next command list in the frame list.
    size_t list_index = command_lists.command_list_indices[command_lists.next_free_index];
    dx12_render_command_list& list = *m_command_lists[list_index];
    list.set_allocated_frame(frame_index);
    command_lists.next_free_index++;

    return list;
}

void dx12_render_command_queue::execute(render_command_list& list)
{
    ID3D12CommandList* const commandLists[] = {
        static_cast<dx12_render_command_list&>(list).get_dx_command_list().Get()
    };
    m_queue->ExecuteCommandLists(1, commandLists);
}

}; // namespace ws
