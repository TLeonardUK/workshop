// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_fence.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"

namespace ws {

vulkan_ri_fence::vulkan_ri_fence(vulkan_render_interface& renderer, const char* debug_name)
    : m_renderer(renderer)
    , m_debug_name(debug_name ? debug_name : "")
{
}

vulkan_ri_fence::~vulkan_ri_fence()
{
    if (m_semaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(m_renderer.get_device(), m_semaphore, nullptr);
        m_semaphore = VK_NULL_HANDLE;
    }
}

result<void> vulkan_ri_fence::create_resources()
{
    VkSemaphoreTypeCreateInfo type_create_info = {};
    type_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    type_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    type_create_info.initialValue = 0;

    VkSemaphoreCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    create_info.pNext = &type_create_info;

    VkResult vk_result = vkCreateSemaphore(m_renderer.get_device(), &create_info, nullptr, &m_semaphore);
    if (!m_renderer.check_result(vk_result, "vkCreateSemaphore"))
    {
        return standard_errors::failed;
    }

    return true;
}

VkSemaphore vulkan_ri_fence::get_semaphore()
{
    return m_semaphore;
}

void vulkan_ri_fence::wait(size_t value)
{
    uint64_t wait_value = static_cast<uint64_t>(value);

    VkSemaphoreWaitInfo wait_info = {};
    wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores = &m_semaphore;
    wait_info.pValues = &wait_value;

    m_renderer.assert_result(vkWaitSemaphores(m_renderer.get_device(), &wait_info, UINT64_MAX), "vkWaitSemaphores");
}

void vulkan_ri_fence::wait(ri_command_queue& queue, size_t value)
{
    uint64_t wait_value = static_cast<uint64_t>(value);

    VkSemaphoreSubmitInfo wait_semaphore_info = {};
    wait_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    wait_semaphore_info.semaphore = m_semaphore;
    wait_semaphore_info.value = wait_value;
    wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkSubmitInfo2 submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.waitSemaphoreInfoCount = 1;
    submit_info.pWaitSemaphoreInfos = &wait_semaphore_info;

    vulkan_ri_command_queue& vk_queue = static_cast<vulkan_ri_command_queue&>(queue);
    m_renderer.assert_result(vkQueueSubmit2(vk_queue.get_queue(), 1, &submit_info, VK_NULL_HANDLE), "vkQueueSubmit2");
}

size_t vulkan_ri_fence::current_value()
{
    uint64_t value = 0;
    vkGetSemaphoreCounterValue(m_renderer.get_device(), m_semaphore, &value);
    return static_cast<size_t>(value);
}

void vulkan_ri_fence::signal(size_t value)
{
    VkSemaphoreSignalInfo signal_info = {};
    signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
    signal_info.semaphore = m_semaphore;
    signal_info.value = static_cast<uint64_t>(value);

    m_renderer.assert_result(vkSignalSemaphore(m_renderer.get_device(), &signal_info), "vkSignalSemaphore");
}

void vulkan_ri_fence::signal(ri_command_queue& queue, size_t value)
{
    VkSemaphoreSubmitInfo signal_semaphore_info = {};
    signal_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signal_semaphore_info.semaphore = m_semaphore;
    signal_semaphore_info.value = static_cast<uint64_t>(value);
    signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkSubmitInfo2 submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.signalSemaphoreInfoCount = 1;
    submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;

    vulkan_ri_command_queue& vk_queue = static_cast<vulkan_ri_command_queue&>(queue);
    m_renderer.assert_result(vkQueueSubmit2(vk_queue.get_queue(), 1, &submit_info, VK_NULL_HANDLE), "vkQueueSubmit2");
}

}; // namespace ws
