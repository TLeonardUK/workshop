// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_list.h"
#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.core/utils/result.h"

#include <array>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace ws {

class vulkan_render_interface;

// ================================================================================================
//  Implementation of a command queue using Vulkan.
// ================================================================================================
class vulkan_ri_command_queue : public ri_command_queue
{
public:
    // Must match vulkan_render_interface::k_max_pipeline_depth.
    constexpr static size_t k_max_pipeline_depth = 3;

    vulkan_ri_command_queue(vulkan_render_interface& renderer, const char* debug_name, uint32_t queue_family);
    virtual ~vulkan_ri_command_queue();

    // Creates the vulkan resources required by this queue.
    result<void> create_resources();

    virtual ri_command_list& alloc_command_list() override;
    virtual void execute(ri_command_list& list) override;
    virtual void execute(const std::vector<ri_command_list*>& list) override;
    virtual void begin_event(const color& color, const char* name, ...) override;
    virtual void end_event() override;

    VkQueue get_queue();
    uint32_t get_queue_family();

    // Called at the start of a new frame, switches the command pools in use and resets
    // recycled pools.
    void begin_frame();

protected:
    struct frame_resources
    {
        VkCommandPool pool = VK_NULL_HANDLE;

        std::vector<size_t> command_list_indices;
        size_t next_free_index = 0;
        size_t last_used_frame_index = 0;
    };

    struct thread_context
    {
        std::array<frame_resources, k_max_pipeline_depth> frames;
        std::vector<std::unique_ptr<vulkan_ri_command_list>> command_lists;
    };

    thread_context& get_thread_context();

private:
    vulkan_render_interface& m_renderer;
    std::string m_debug_name;
    uint32_t m_queue_family;

    VkQueue m_queue = VK_NULL_HANDLE;

    // Guards vkQueueSubmit2 calls against this queue - a VkQueue must be externally synchronized
    // across submissions per the Vulkan spec, and execute() can be re-entered (eg. via
    // flush_uploads() flushing queued uploads/transitions before the caller's own submission).
    std::mutex m_queue_submit_mutex;

    std::recursive_mutex m_thread_context_mutex;
    std::unordered_map<std::thread::id, std::unique_ptr<thread_context>> m_thread_contexts;

    size_t m_frame_index = 0;

    PFN_vkQueueBeginDebugUtilsLabelEXT m_begin_label_fn = nullptr;
    PFN_vkQueueEndDebugUtilsLabelEXT m_end_label_fn = nullptr;

};

}; // namespace ws
