// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_command_list.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.windowing/window.h"

#include "thirdparty/pix/include/pix3.h"

namespace ws {

dx12_ri_command_queue::dx12_ri_command_queue(dx12_render_interface& renderer, const char* debug_name, D3D12_COMMAND_LIST_TYPE queue_type)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_queue_type(queue_type)
{
}

dx12_ri_command_queue::~dx12_ri_command_queue()
{
    for (auto& [thread_id, context] : m_thread_contexts)
    {
        for (auto& resources : context->frame_resources)
        {
            SafeRelease(resources.allocator);
        }
    }

    m_thread_contexts.clear();

    SafeRelease(m_queue);
}

result<void> dx12_ri_command_queue::create_resources()
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

    return true;
}

dx12_ri_command_queue::thread_context& dx12_ri_command_queue::get_thread_context()
{
    std::scoped_lock lock(m_thread_context_mutex);

    std::thread::id thread_id = std::this_thread::get_id();

    // Create allocator if one is not already assigned to this thread.
    if (auto iter = m_thread_contexts.find(thread_id); iter == m_thread_contexts.end())
    {
        std::unique_ptr<thread_context> context = std::make_unique<thread_context>();

        for (size_t i = 0; i < dx12_render_interface::k_max_pipeline_depth; i++)
        {
            HRESULT hr = m_renderer.get_device()->CreateCommandAllocator(m_queue_type, IID_PPV_ARGS(&context->frame_resources[i].allocator));
            if (FAILED(hr))
            {
                db_fatal(render_interface, "CreateCommandAllocator failed with error 0x%08x.", hr);
            }
        }

        frame_resources& resources = context->frame_resources[m_frame_index % dx12_render_interface::k_max_pipeline_depth];
        resources.last_used_frame_index = m_frame_index;

        m_thread_contexts.emplace(thread_id, std::move(context));
    }

    return *m_thread_contexts[thread_id].get();
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> dx12_ri_command_queue::get_queue()
{
    return m_queue;
}

D3D12_COMMAND_LIST_TYPE dx12_ri_command_queue::get_dx_queue_type()
{
    return m_queue_type;
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> dx12_ri_command_queue::get_current_command_allocator()
{
    thread_context& context = get_thread_context();
    frame_resources& resources = context.frame_resources[m_frame_index % dx12_render_interface::k_max_pipeline_depth];
    return resources.allocator;
}

void dx12_ri_command_queue::begin_frame()
{
    std::scoped_lock lock(m_thread_context_mutex);

    m_frame_index = m_renderer.get_frame_index();

    begin_event(profile_colors::gpu_frame, "frame %zi", m_frame_index);

    // Reset resources of all previous frames.
    for (auto& [thread_id, context] : m_thread_contexts)
    {
        frame_resources& resources = context->frame_resources[m_frame_index % dx12_render_interface::k_max_pipeline_depth];

        if (resources.last_used_frame_index != m_frame_index)
        {
            resources.allocator->Reset();

            for (size_t i = 0; i < resources.next_free_index; i++)
            {
                db_assert_message(!context->command_lists[i]->is_open(), "Reusing command list that hasn't been closed. Command lists should only remain open for the duration of the frame they are allocated on.");
            }

            resources.next_free_index = 0;
            resources.last_used_frame_index = m_frame_index;
        }
    }
}


void dx12_ri_command_queue::end_frame()
{
    end_event();
}

ri_command_list& dx12_ri_command_queue::alloc_command_list()
{
    thread_context& context = get_thread_context();
    frame_resources& resources = context.frame_resources[m_frame_index % dx12_render_interface::k_max_pipeline_depth];

    size_t frame_index = m_renderer.get_frame_index();

    // Ensure allocator has been reset to this frame.
    db_assert(resources.last_used_frame_index == m_frame_index);

    // Allocate new command list if we have no more available.
    if (resources.next_free_index >= resources.command_list_indices.size())
    {
        std::string debug_name = string_format("Command List [index=%zi]", context.command_lists.size());

        std::unique_ptr<dx12_ri_command_list> list = std::make_unique<dx12_ri_command_list>(m_renderer, debug_name.c_str(), *this);
        if (!list->create_resources())
        {
            db_fatal(render_interface, "Failed to create command list resources.");
        }

        context.command_lists.push_back(std::move(list));
        resources.command_list_indices.push_back(context.command_lists.size() - 1);
    }

    // Return next command list in the frame list.
    size_t list_index = resources.command_list_indices[resources.next_free_index];
    dx12_ri_command_list& list = *context.command_lists[list_index];
    list.set_allocated_frame(frame_index);
    resources.next_free_index++;

    return list;
}

void dx12_ri_command_queue::execute(ri_command_list& list)
{
    ID3D12CommandList* const commandLists[] = {
        static_cast<dx12_ri_command_list&>(list).get_dx_command_list().Get()
    };
    m_queue->ExecuteCommandLists(1, commandLists);
}

void dx12_ri_command_queue::begin_event(const color& color, const char* format, ...)
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

    PIXBeginEvent(m_queue.Get(), PIX_COLOR(r, g, b), "%s", buffer);

    va_end(list);
}

void dx12_ri_command_queue::end_event()
{
    PIXEndEvent(m_queue.Get());
}

}; // namespace ws
