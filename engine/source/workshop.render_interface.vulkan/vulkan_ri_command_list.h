// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.core/utils/result.h"

#include <string>
#include <vector>

namespace ws {

class ri_param_block;
class vulkan_render_interface;
class vulkan_ri_command_queue;
class vulkan_ri_pipeline;

// ================================================================================================
//  Implementation of a command list using Vulkan.
// ================================================================================================
class vulkan_ri_command_list : public ri_command_list
{
public:
    vulkan_ri_command_list(vulkan_render_interface& renderer, const char* debug_name, vulkan_ri_command_queue& queue, VkCommandPool pool);
    virtual ~vulkan_ri_command_list();

    // Creates the vulkan resources required by this command list.
    result<void> create_resources();

    virtual void open() override;
    virtual void close() override;
    virtual void barrier(ri_texture& resource, ri_resource_state source_state, ri_resource_state destination_state) override;
    virtual void barrier(ri_buffer& resource, ri_resource_state source_state, ri_resource_state destination_state) override;
    virtual void clear(ri_texture_view resource, const color& destination) override;
    virtual void clear_depth(ri_texture_view resource, float depth, size_t stencil) override;
    virtual void set_pipeline(ri_pipeline& pipeline) override;
    virtual void set_param_blocks(const std::vector<ri_param_block*> param_blocks) override;
    virtual void set_viewport(const recti& rect) override;
    virtual void set_scissor(const recti& rect) override;
    virtual void set_blend_factor(const vector4& factor) override;
    virtual void set_stencil_ref(uint32_t value) override;
    virtual void set_primitive_topology(ri_primitive value) override;
    virtual void set_index_buffer(ri_buffer& buffer) override;
    virtual void set_render_targets(const std::vector<ri_texture_view>& colors, ri_texture_view depth) override;
    virtual void draw(size_t indexes_per_instance, size_t instance_count, size_t start_index_location = 0) override;
    virtual void dispatch(size_t group_size_x, size_t group_size_y, size_t group_size_z) override;
    virtual void dispatch_rays(size_t group_size_x, size_t group_size_y, size_t group_size_z) override;
    virtual void begin_event(const color& color, const char* name, ...) override;
    virtual void end_event() override;
    virtual void begin_query(ri_query* query) override;
    virtual void end_query(ri_query* query) override;
    virtual void copy_texture(ri_texture* texture, ri_buffer* buffer) override;
    virtual void copy_buffer(ri_buffer* destination, ri_buffer* source) override;
    virtual void clear_buffer(ri_buffer* destination) override;


    // Inserts a barrier against a raw vulkan image (used by resources that aren't ri_textures,
    // such as swapchain-owned images).
    void barrier(VkImage image, ri_resource_state source_state, ri_resource_state destination_state, VkImageAspectFlags aspect, uint32_t base_mip = 0, uint32_t mip_count = VK_REMAINING_MIP_LEVELS);

    VkCommandBuffer get_command_buffer();

    bool is_open();
    void set_allocated_frame(size_t frame);

private:
    vulkan_render_interface& m_renderer;
    std::string m_debug_name;
    vulkan_ri_command_queue& m_queue;
    VkCommandPool m_pool;

    bool m_opened = false;
    size_t m_allocated_frame_index = 0;

    bool m_rendering_active = false;

    vulkan_ri_pipeline* m_active_pipeline = nullptr;

    VkCommandBuffer m_command_buffer = VK_NULL_HANDLE;

    PFN_vkCmdBeginDebugUtilsLabelEXT m_begin_label_fn = nullptr;
    PFN_vkCmdEndDebugUtilsLabelEXT m_end_label_fn = nullptr;

};

}; // namespace ws
