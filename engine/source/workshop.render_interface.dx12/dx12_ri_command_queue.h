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

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> get_queue();
    D3D12_COMMAND_LIST_TYPE get_dx_queue_type();

    // Gets the command allocator for the current frame begin built.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> get_current_command_allocator();

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;
    D3D12_COMMAND_LIST_TYPE m_queue_type;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue = nullptr;

    struct frame_command_lists
    {
        std::vector<size_t> command_list_indices;
        size_t next_free_index = 0;
        size_t last_used_frame_index = 0;
    };

    std::mutex m_command_list_mutex;
    std::vector<std::unique_ptr<dx12_ri_command_list>> m_command_lists;
    std::array<frame_command_lists, dx12_render_interface::k_max_pipeline_depth> m_frame_command_lists;

    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, dx12_render_interface::k_max_pipeline_depth> m_command_allocators;

};

}; // namespace ws
