// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_command_list.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_ri_pipeline.h"
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_table.h"
#include "workshop.render_interface.vulkan/vulkan_ri_query.h"
#include "workshop.render_interface.vulkan/vulkan_ri_texture.h"
#include "workshop.render_interface.vulkan/vulkan_ri_param_block.h"
#include "workshop.render_interface.vulkan/vulkan_types.h"

#include <array>

namespace ws {

vulkan_ri_command_list::vulkan_ri_command_list(vulkan_render_interface& renderer, const char* debug_name, vulkan_ri_command_queue& queue, VkCommandPool pool)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_queue(queue)
    , m_pool(pool)
{
}

vulkan_ri_command_list::~vulkan_ri_command_list()
{
    if (m_command_buffer != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(m_renderer.get_device(), m_pool, 1, &m_command_buffer);
        m_command_buffer = VK_NULL_HANDLE;
    }
}

result<void> vulkan_ri_command_list::create_resources()
{
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = m_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkResult vk_result = vkAllocateCommandBuffers(m_renderer.get_device(), &allocate_info, &m_command_buffer);
    if (!m_renderer.check_result(vk_result, "vkAllocateCommandBuffers"))
    {
        return standard_errors::failed;
    }

    m_begin_label_fn = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
        vkGetInstanceProcAddr(m_renderer.get_instance(), "vkCmdBeginDebugUtilsLabelEXT"));
    m_end_label_fn = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
        vkGetInstanceProcAddr(m_renderer.get_instance(), "vkCmdEndDebugUtilsLabelEXT"));

    return true;
}

VkCommandBuffer vulkan_ri_command_list::get_command_buffer()
{
    return m_command_buffer;
}

bool vulkan_ri_command_list::is_open()
{
    return m_opened;
}

void vulkan_ri_command_list::set_allocated_frame(size_t frame)
{
    m_allocated_frame_index = frame;
}

void vulkan_ri_command_list::open()
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    m_renderer.assert_result(vkBeginCommandBuffer(m_command_buffer, &begin_info), "vkBeginCommandBuffer");

    m_opened = true;
    m_active_pipeline = nullptr;
    m_rendering_active = false;
}

void vulkan_ri_command_list::close()
{
    if (m_rendering_active)
    {
        vkCmdEndRendering(m_command_buffer);
        m_rendering_active = false;
    }

    m_renderer.assert_result(vkEndCommandBuffer(m_command_buffer), "vkEndCommandBuffer");

    m_opened = false;
}

void vulkan_ri_command_list::barrier(VkImage image, ri_resource_state source_state, ri_resource_state destination_state, VkImageAspectFlags aspect, uint32_t base_mip, uint32_t mip_count)
{
    // Vulkan only allows global memory barriers inside an active dynamic rendering scope, not
    // image/buffer barriers - unlike DX12, which has no equivalent scoping restriction. Most
    // callers (eg. the final backbuffer -> present transition after the last draw of a frame)
    // have no reason to know or care whether rendering is still active, so close it here rather
    // than pushing that responsibility onto every barrier call site.
    if (m_rendering_active)
    {
        vkCmdEndRendering(m_command_buffer);
        m_rendering_active = false;
    }

    ri_record_image_barrier(m_command_buffer, image, source_state, destination_state, aspect, base_mip, mip_count);
}

void vulkan_ri_command_list::barrier(ri_texture& resource, ri_resource_state source_state, ri_resource_state destination_state)
{
    vulkan_ri_texture& texture = static_cast<vulkan_ri_texture&>(resource);

    // "initial" is a placeholder for "whatever this resource's own common state is", not a
    // real vulkan layout - resolve it per-texture, same as dx12_ri_command_list::barrier does.
    // Exception: a texture's VkImage always starts out in VK_IMAGE_LAYOUT_UNDEFINED regardless
    // of what its conceptual "common" state is (unlike a D3D12 resource, which genuinely starts
    // in its creation-specified state) - so the very first transition's source must be left as
    // ri_resource_state::initial (which maps to VK_IMAGE_LAYOUT_UNDEFINED) rather than resolved.
    if (source_state == ri_resource_state::initial && !texture.has_undefined_layout())
    {
        source_state = texture.get_initial_state();
    }
    if (destination_state == ri_resource_state::initial)
    {
        destination_state = texture.get_initial_state();
    }

    if (source_state == destination_state)
    {
        return;
    }

    texture.clear_undefined_layout();

    barrier(texture.get_image(), source_state, destination_state, texture.get_aspect_mask());
}

void vulkan_ri_command_list::barrier(ri_buffer& resource, ri_resource_state source_state, ri_resource_state destination_state)
{
    // See the equivalent check in the VkImage overload above.
    if (m_rendering_active)
    {
        vkCmdEndRendering(m_command_buffer);
        m_rendering_active = false;
    }

    vulkan_ri_buffer& buffer = static_cast<vulkan_ri_buffer&>(resource);

    vulkan_buffer_state src = ri_to_vulkan_buffer_state(source_state);
    vulkan_buffer_state dst = ri_to_vulkan_buffer_state(destination_state);

    VkBufferMemoryBarrier2 barrier_info = {};
    barrier_info.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier_info.srcStageMask = src.stage;
    barrier_info.srcAccessMask = src.access;
    barrier_info.dstStageMask = dst.stage;
    barrier_info.dstAccessMask = dst.access;
    barrier_info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier_info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier_info.buffer = buffer.get_buffer();
    barrier_info.offset = buffer.get_buffer_offset();
    barrier_info.size = buffer.get_element_count() * buffer.get_element_size();

    VkDependencyInfo dependency_info = {};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.bufferMemoryBarrierCount = 1;
    dependency_info.pBufferMemoryBarriers = &barrier_info;

    vkCmdPipelineBarrier2(m_command_buffer, &dependency_info);
}

void vulkan_ri_command_list::clear(ri_texture_view resource, const color& destination)
{
    vulkan_ri_texture& texture = static_cast<vulkan_ri_texture&>(*resource.texture);

    size_t mip = (resource.mip == ri_texture_view::k_unset) ? 0 : resource.mip;
    size_t slice = (resource.slice == ri_texture_view::k_unset) ? 0 : resource.slice;

    VkRenderingAttachmentInfo attachment = {};
    attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachment.imageView = texture.get_rtv_view(slice, mip);
    attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea.extent = { static_cast<uint32_t>(texture.get_width()), static_cast<uint32_t>(texture.get_height()) };
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &attachment;

    bool need_scope = !m_rendering_active;
    if (need_scope)
    {
        vkCmdBeginRendering(m_command_buffer, &rendering_info);
    }

    VkClearAttachment clear_attachment = {};
    clear_attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    clear_attachment.colorAttachment = 0;
    clear_attachment.clearValue.color.float32[0] = destination.r;
    clear_attachment.clearValue.color.float32[1] = destination.g;
    clear_attachment.clearValue.color.float32[2] = destination.b;
    clear_attachment.clearValue.color.float32[3] = destination.a;

    VkClearRect clear_rect = {};
    clear_rect.rect = rendering_info.renderArea;
    clear_rect.layerCount = 1;

    vkCmdClearAttachments(m_command_buffer, 1, &clear_attachment, 1, &clear_rect);

    if (need_scope)
    {
        vkCmdEndRendering(m_command_buffer);
    }
}

void vulkan_ri_command_list::clear_depth(ri_texture_view resource, float depth, size_t stencil)
{
    vulkan_ri_texture& texture = static_cast<vulkan_ri_texture&>(*resource.texture);

    size_t slice = (resource.slice == ri_texture_view::k_unset) ? 0 : resource.slice;

    VkRenderingAttachmentInfo depth_attachment = {};
    depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth_attachment.imageView = texture.get_dsv_view(slice);
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea.extent = { static_cast<uint32_t>(texture.get_width()), static_cast<uint32_t>(texture.get_height()) };
    rendering_info.layerCount = 1;
    rendering_info.pDepthAttachment = &depth_attachment;
    if (texture.get_format() == ri_texture_format::D24_UNORM_S8_UINT)
    {
        rendering_info.pStencilAttachment = &depth_attachment;
    }

    bool need_scope = !m_rendering_active;
    if (need_scope)
    {
        vkCmdBeginRendering(m_command_buffer, &rendering_info);
    }

    VkClearAttachment clear_attachment = {};
    clear_attachment.aspectMask = texture.get_aspect_mask();
    clear_attachment.clearValue.depthStencil.depth = depth;
    clear_attachment.clearValue.depthStencil.stencil = static_cast<uint32_t>(stencil);

    VkClearRect clear_rect = {};
    clear_rect.rect = rendering_info.renderArea;
    clear_rect.layerCount = 1;

    vkCmdClearAttachments(m_command_buffer, 1, &clear_attachment, 1, &clear_rect);

    if (need_scope)
    {
        vkCmdEndRendering(m_command_buffer);
    }
}

void vulkan_ri_command_list::set_pipeline(ri_pipeline& pipeline)
{
    vulkan_ri_pipeline& vk_pipeline = static_cast<vulkan_ri_pipeline&>(pipeline);
    m_active_pipeline = &vk_pipeline;

    // Real VkPipeline creation is blocked on vulkan_ri_shader_compiler producing SPIR-V
    // bytecode (see vulkan_ri_pipeline::create_resources) - nothing to bind yet.
    VkPipeline vk_handle = vk_pipeline.get_pipeline();
    if (vk_handle == VK_NULL_HANDLE)
    {
        return;
    }

    VkPipelineBindPoint bind_point = vk_pipeline.get_bind_point();
    vkCmdBindPipeline(m_command_buffer, bind_point, vk_handle);

    std::array<VkDescriptorSet, vulkan_render_interface::k_bindless_set_count> sets;
    for (size_t i = 0; i < vulkan_render_interface::k_bindless_set_count; i++)
    {
        sets[i] = m_renderer.get_descriptor_table(static_cast<ri_descriptor_table>(i)).get_descriptor_set();
    }

    vkCmdBindDescriptorSets(m_command_buffer, bind_point, vk_pipeline.get_pipeline_layout(), 1, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
}

void vulkan_ri_command_list::set_param_blocks(const std::vector<ri_param_block*> param_blocks)
{
    db_assert(m_active_pipeline != nullptr);

    const auto& archetype_list = m_active_pipeline->get_create_params().param_block_archetypes;

    for (size_t i = 0; i < archetype_list.size(); i++)
    {
        ri_param_block_archetype* archetype = archetype_list[i];

        // Instance param blocks are passed in via the instant buffer. We don't need to provide them here.
        if (archetype->get_create_params().scope == ri_data_scope::instance ||
            archetype->get_create_params().scope == ri_data_scope::indirect)
        {
            continue;
        }

        size_t push_constant_offset = 0;
        if (!m_active_pipeline->get_push_constant_offset(archetype, push_constant_offset))
        {
            continue;
        }

        // Find appropriate param block in input.
        bool found = false;

        for (ri_param_block* base : param_blocks)
        {
            vulkan_ri_param_block* input = static_cast<vulkan_ri_param_block*>(base);
            if (input->get_archetype() == archetype)
            {
                VkDeviceAddress address = input->consume();

                vkCmdPushConstants(
                    m_command_buffer,
                    m_active_pipeline->get_pipeline_layout(),
                    VK_SHADER_STAGE_ALL,
                    static_cast<uint32_t>(push_constant_offset),
                    static_cast<uint32_t>(sizeof(VkDeviceAddress)),
                    &address
                );

                found = true;
                break;
            }
        }

        if (!found)
        {
            db_error(render_interface, "set_param_blocks didn't include param block expected by pipeline '%s'.", archetype->get_name());
            return;
        }
    }
}

void vulkan_ri_command_list::set_viewport(const recti& rect)
{
    VkViewport viewport = {};
    viewport.x = static_cast<float>(rect.x);
    viewport.y = static_cast<float>(rect.y);
    viewport.width = static_cast<float>(rect.width);
    viewport.height = static_cast<float>(rect.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(m_command_buffer, 0, 1, &viewport);
}

void vulkan_ri_command_list::set_scissor(const recti& rect)
{
    VkRect2D scissor = {};
    scissor.offset.x = rect.x;
    scissor.offset.y = rect.y;
    scissor.extent.width = static_cast<uint32_t>(rect.width);
    scissor.extent.height = static_cast<uint32_t>(rect.height);

    vkCmdSetScissor(m_command_buffer, 0, 1, &scissor);
}

void vulkan_ri_command_list::set_blend_factor(const vector4& factor)
{
    float constants[4] = { factor.x, factor.y, factor.z, factor.w };
    vkCmdSetBlendConstants(m_command_buffer, constants);
}

void vulkan_ri_command_list::set_stencil_ref(uint32_t value)
{
    vkCmdSetStencilReference(m_command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, value);
}

void vulkan_ri_command_list::set_primitive_topology(ri_primitive value)
{
    vkCmdSetPrimitiveTopology(m_command_buffer, ri_to_vulkan(value));
}

void vulkan_ri_command_list::set_index_buffer(ri_buffer& buffer)
{
    vulkan_ri_buffer& vk_buffer = static_cast<vulkan_ri_buffer&>(buffer);

    VkIndexType index_type = (vk_buffer.get_element_size() == sizeof(uint16_t)) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;

    vkCmdBindIndexBuffer(m_command_buffer, vk_buffer.get_buffer(), vk_buffer.get_buffer_offset(), index_type);
}

void vulkan_ri_command_list::set_render_targets(const std::vector<ri_texture_view>& colors, ri_texture_view depth)
{
    if (m_rendering_active)
    {
        vkCmdEndRendering(m_command_buffer);
        m_rendering_active = false;
    }

    std::vector<VkRenderingAttachmentInfo> color_attachments;
    VkExtent2D render_extent = {};

    for (ri_texture_view value : colors)
    {
        vulkan_ri_texture* vk_tex = static_cast<vulkan_ri_texture*>(value.texture);

        size_t slice = (value.slice == ri_texture_view::k_unset) ? 0 : value.slice;
        size_t mip = (value.mip == ri_texture_view::k_unset) ? 0 : value.mip;

        VkRenderingAttachmentInfo attachment = {};
        attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachment.imageView = vk_tex->get_rtv_view(slice, mip);
        attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        color_attachments.push_back(attachment);

        render_extent.width = static_cast<uint32_t>(vk_tex->get_width());
        render_extent.height = static_cast<uint32_t>(vk_tex->get_height());
    }

    VkRenderingAttachmentInfo depth_attachment = {};
    bool has_depth = (depth.texture != nullptr);
    if (has_depth)
    {
        db_assert(depth.mip == 0 || depth.mip == ri_texture_view::k_unset);

        vulkan_ri_texture* vk_tex = static_cast<vulkan_ri_texture*>(depth.texture);

        size_t slice = (depth.slice == ri_texture_view::k_unset) ? 0 : depth.slice;

        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment.imageView = vk_tex->get_dsv_view(slice);
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        render_extent.width = static_cast<uint32_t>(vk_tex->get_width());
        render_extent.height = static_cast<uint32_t>(vk_tex->get_height());
    }

    VkRenderingInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea.extent = render_extent;
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = static_cast<uint32_t>(color_attachments.size());
    rendering_info.pColorAttachments = color_attachments.data();
    if (has_depth)
    {
        rendering_info.pDepthAttachment = &depth_attachment;

        vulkan_ri_texture* vk_tex = static_cast<vulkan_ri_texture*>(depth.texture);
        if (vk_tex->get_format() == ri_texture_format::D24_UNORM_S8_UINT)
        {
            rendering_info.pStencilAttachment = &depth_attachment;
        }
    }

    vkCmdBeginRendering(m_command_buffer, &rendering_info);
    m_rendering_active = true;
}

void vulkan_ri_command_list::draw(size_t indexes_per_instance, size_t instance_count, size_t start_index_location)
{
    vkCmdDrawIndexed(m_command_buffer, static_cast<uint32_t>(indexes_per_instance), static_cast<uint32_t>(instance_count), static_cast<uint32_t>(start_index_location), 0, 0);
}

void vulkan_ri_command_list::dispatch(size_t group_size_x, size_t group_size_y, size_t group_size_z)
{
    vkCmdDispatch(m_command_buffer, static_cast<uint32_t>(group_size_x), static_cast<uint32_t>(group_size_y), static_cast<uint32_t>(group_size_z));
}

void vulkan_ri_command_list::dispatch_rays(size_t group_size_x, size_t group_size_y, size_t group_size_z)
{
    VkStridedDeviceAddressRegionKHR raygen_record = m_active_pipeline->get_ray_generation_shader_record();
    VkStridedDeviceAddressRegionKHR miss_table = m_active_pipeline->get_miss_shader_table();
    VkStridedDeviceAddressRegionKHR hit_group_table = m_active_pipeline->get_hit_group_table();
    VkStridedDeviceAddressRegionKHR callable_table = m_active_pipeline->get_callable_shader_table();

    m_renderer.vkCmdTraceRaysKHR_fn(
        m_command_buffer,
        &raygen_record,
        &miss_table,
        &hit_group_table,
        &callable_table,
        static_cast<uint32_t>(group_size_x),
        static_cast<uint32_t>(group_size_y),
        static_cast<uint32_t>(group_size_z)
    );
}

void vulkan_ri_command_list::begin_event(const color& color, const char* format, ...)
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

    m_begin_label_fn(m_command_buffer, &label);
}

void vulkan_ri_command_list::end_event()
{
    if (m_end_label_fn == nullptr)
    {
        return;
    }

    m_end_label_fn(m_command_buffer);
}

void vulkan_ri_command_list::begin_query(ri_query* query)
{
    static_cast<vulkan_ri_query*>(query)->begin(*this);
}

void vulkan_ri_command_list::end_query(ri_query* query)
{
    static_cast<vulkan_ri_query*>(query)->end(*this);
}

void vulkan_ri_command_list::copy_texture(ri_texture* texture, ri_buffer* buffer)
{
    size_t required_space = texture->get_width() * texture->get_height() * ri_bytes_per_texel(texture->get_format());
    db_assert(buffer->get_element_count() * buffer->get_element_size() >= required_space);

    vulkan_ri_texture* vk_texture = static_cast<vulkan_ri_texture*>(texture);
    vulkan_ri_buffer* vk_buffer = static_cast<vulkan_ri_buffer*>(buffer);

    VkBufferImageCopy2 region = {};
    region.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
    region.bufferOffset = vk_buffer->get_buffer_offset();
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk_texture->get_aspect_mask();
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { static_cast<uint32_t>(texture->get_width()), static_cast<uint32_t>(texture->get_height()), 1 };

    VkCopyImageToBufferInfo2 copy_info = {};
    copy_info.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2;
    copy_info.srcImage = vk_texture->get_image();
    copy_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    copy_info.dstBuffer = vk_buffer->get_buffer();
    copy_info.regionCount = 1;
    copy_info.pRegions = &region;

    vkCmdCopyImageToBuffer2(m_command_buffer, &copy_info);
}

void vulkan_ri_command_list::copy_buffer(ri_buffer* destination, ri_buffer* source)
{

}

void vulkan_ri_command_list::clear_buffer(ri_buffer* destination)
{ 

}

}; // namespace ws
