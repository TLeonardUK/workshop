// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_list.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.core/containers/string.h"

namespace ws {

vulkan_ri_command_queue::vulkan_ri_command_queue(vulkan_render_interface& renderer, const char* debug_name, uint32_t queue_family)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_queue_family(queue_family)
{
}

vulkan_ri_command_queue::~vulkan_ri_command_queue()
{
    VkDevice device = m_renderer.get_device();

    for (auto& [thread_id, context] : m_thread_contexts)
    {
        for (frame_resources& resources : context->frames)
        {
            if (resources.pool != VK_NULL_HANDLE)
            {
                vkDestroyCommandPool(device, resources.pool, nullptr);
            }
        }
    }

    m_thread_contexts.clear();
}

result<void> vulkan_ri_command_queue::create_resources()
{
    vkGetDeviceQueue(m_renderer.get_device(), m_queue_family, 0, &m_queue);

    m_begin_label_fn = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(
        vkGetInstanceProcAddr(m_renderer.get_instance(), "vkQueueBeginDebugUtilsLabelEXT"));
    m_end_label_fn = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(
        vkGetInstanceProcAddr(m_renderer.get_instance(), "vkQueueEndDebugUtilsLabelEXT"));

    return true;
}

vulkan_ri_command_queue::thread_context& vulkan_ri_command_queue::get_thread_context()
{
    std::scoped_lock lock(m_thread_context_mutex);

    std::thread::id thread_id = std::this_thread::get_id();

    if (auto iter = m_thread_contexts.find(thread_id); iter == m_thread_contexts.end())
    {
        std::unique_ptr<thread_context> context = std::make_unique<thread_context>();

        VkCommandPoolCreateInfo pool_create_info = {};
        pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        pool_create_info.queueFamilyIndex = m_queue_family;

        for (size_t i = 0; i < k_max_pipeline_depth; i++)
        {
            VkResult vk_result = vkCreateCommandPool(m_renderer.get_device(), &pool_create_info, nullptr, &context->frames[i].pool);
            m_renderer.assert_result(vk_result, "vkCreateCommandPool");
        }

        frame_resources& resources = context->frames[m_frame_index % k_max_pipeline_depth];
        resources.last_used_frame_index = m_frame_index;

        m_thread_contexts.emplace(thread_id, std::move(context));
    }

    return *m_thread_contexts[thread_id].get();
}

VkQueue vulkan_ri_command_queue::get_queue()
{
    return m_queue;
}

uint32_t vulkan_ri_command_queue::get_queue_family()
{
    return m_queue_family;
}

void vulkan_ri_command_queue::begin_frame()
{
    std::scoped_lock lock(m_thread_context_mutex);

    m_frame_index = m_renderer.get_frame_index();

    begin_event(profile_colors::gpu_frame, "frame %zi", m_frame_index);

    profile_marker(profile_colors::render, "Reset Command Queue Pools");

    for (auto& [thread_id, context] : m_thread_contexts)
    {
        frame_resources& resources = context->frames[m_frame_index % k_max_pipeline_depth];

        if (resources.last_used_frame_index != m_frame_index)
        {
            vkResetCommandPool(m_renderer.get_device(), resources.pool, 0);

            for (size_t i = 0; i < resources.next_free_index; i++)
            {
                db_assert_message(!context->command_lists[resources.command_list_indices[i]]->is_open(), "Reusing command list that hasn't been closed. Command lists should only remain open for the duration of the frame they are allocated on.");
            }

            resources.next_free_index = 0;
            resources.last_used_frame_index = m_frame_index;
        }
    }
}

ri_command_list& vulkan_ri_command_queue::alloc_command_list()
{
    thread_context& context = get_thread_context();
    frame_resources& resources = context.frames[m_frame_index % k_max_pipeline_depth];

    size_t frame_index = m_renderer.get_frame_index();

    db_assert(resources.last_used_frame_index == m_frame_index);

    if (resources.next_free_index >= resources.command_list_indices.size())
    {
        std::string debug_name = string_format("Command List [index=%zi]", context.command_lists.size());

        std::unique_ptr<vulkan_ri_command_list> list = std::make_unique<vulkan_ri_command_list>(m_renderer, debug_name.c_str(), *this, resources.pool);
        if (!list->create_resources())
        {
            db_fatal(render_interface, "Failed to create command list resources.");
        }

        context.command_lists.push_back(std::move(list));
        resources.command_list_indices.push_back(context.command_lists.size() - 1);
    }

    size_t list_index = resources.command_list_indices[resources.next_free_index];
    vulkan_ri_command_list& list = *context.command_lists[list_index];
    list.set_allocated_frame(frame_index);
    resources.next_free_index++;

    return list;
}

void vulkan_ri_command_queue::execute(ri_command_list& list)
{
    m_renderer.flush_uploads();

    vulkan_ri_command_list& vk_list = static_cast<vulkan_ri_command_list&>(list);

    VkCommandBufferSubmitInfo command_buffer_submit_info = {};
    command_buffer_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    command_buffer_submit_info.commandBuffer = vk_list.get_command_buffer();

    VkSubmitInfo2 submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &command_buffer_submit_info;

    std::scoped_lock lock(m_queue_submit_mutex);
    m_renderer.assert_result(vkQueueSubmit2(m_queue, 1, &submit_info, VK_NULL_HANDLE), "vkQueueSubmit2");
}

void vulkan_ri_command_queue::execute(const std::vector<ri_command_list*>& lists)
{
    m_renderer.flush_uploads();

    std::vector<VkCommandBufferSubmitInfo> command_buffer_submit_infos;
    command_buffer_submit_infos.reserve(lists.size());

    for (ri_command_list* list : lists)
    {
        VkCommandBufferSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        info.commandBuffer = static_cast<vulkan_ri_command_list*>(list)->get_command_buffer();
        command_buffer_submit_infos.push_back(info);
    }

    VkSubmitInfo2 submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.commandBufferInfoCount = static_cast<uint32_t>(command_buffer_submit_infos.size());
    submit_info.pCommandBufferInfos = command_buffer_submit_infos.data();

    std::scoped_lock lock(m_queue_submit_mutex);
    m_renderer.assert_result(vkQueueSubmit2(m_queue, 1, &submit_info, VK_NULL_HANDLE), "vkQueueSubmit2");
}

void vulkan_ri_command_queue::begin_event(const color& color, const char* format, ...)
{
    if (m_begin_label_fn == nullptr)
    {
        return;
    }

    uint8_t r, g, b, a;
    color.get(r, g, b, a);

    char buffer[1024];

    va_list list;
    va_start(list, format);
    int ret = vsnprintf(buffer, sizeof(buffer), format, list);
    va_end(list);

    if (ret < 0 || static_cast<size_t>(ret) >= sizeof(buffer))
    {
        return;
    }

    VkDebugUtilsLabelEXT label = {};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = buffer;
    label.color[0] = r / 255.0f;
    label.color[1] = g / 255.0f;
    label.color[2] = b / 255.0f;
    label.color[3] = a / 255.0f;

    m_begin_label_fn(m_queue, &label);
}

void vulkan_ri_command_queue::end_event()
{
    if (m_end_label_fn == nullptr)
    {
        return;
    }

    m_end_label_fn(m_queue);
}

}; // namespace ws
