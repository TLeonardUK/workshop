// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_list.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Implementation of a command queue using DirectX 12.
// ================================================================================================
class dx12_ri_command_queue : public ri_command_queue
{
public:
    dx12_ri_command_queue(dx12_render_interface& renderer, const char* debug_name, D3D12_COMMAND_LIST_TYPE queue_type);
    virtual ~dx12_ri_command_queue();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    virtual ri_command_list& alloc_command_list() override;
    virtual void execute(ri_command_list& list) override;
    virtual void execute(const std::vector<ri_command_list*>& list) override;
    virtual void begin_event(const color& color, const char* name, ...) override;
    virtual void end_event() override;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> get_queue();
    D3D12_COMMAND_LIST_TYPE get_dx_queue_type();

    // Gets the command allocator for the current frame begin built.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> get_current_command_allocator();

    // Called at the start of a new frame, switches the command list allocators in use
    // and resets recycled allocators.
    void begin_frame();

    // Called when a frame finishes rendering.
    void end_frame();

protected:
    struct frame_resources
    {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;

        std::vector<size_t> command_list_indices;
        size_t next_free_index = 0;
        size_t last_used_frame_index = 0;
    };

    struct thread_context
    {
        std::array<frame_resources, dx12_render_interface::k_max_pipeline_depth> frame_resources;
        std::vector<std::unique_ptr<dx12_ri_command_list>> command_lists;
    };

    // Gets the allocation context for the current thread.
    thread_context& get_thread_context();

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;
    D3D12_COMMAND_LIST_TYPE m_queue_type;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue = nullptr;

    std::recursive_mutex m_thread_context_mutex;
    std::unordered_map<std::thread::id, std::unique_ptr<thread_context>> m_thread_contexts;

    size_t m_frame_index = 0;

};

}; // namespace ws
