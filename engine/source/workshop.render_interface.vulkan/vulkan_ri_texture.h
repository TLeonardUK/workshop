// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_table.h"
#include "workshop.render_interface.vulkan/vulkan_ri_tile_manager.h"
#include "workshop.core/memory/memory_tracker.h"
#include "workshop.core/utils/result.h"

#include <array>
#include <mutex>
#include <string>
#include <vector>

namespace ws {

class vulkan_render_interface;
class vulkan_ri_param_block;
class vulkan_ri_staging_buffer;

// ================================================================================================
//  Implementation of a texture buffer using Vulkan.
// ================================================================================================
class vulkan_ri_texture : public ri_texture
{
public:
    vulkan_ri_texture(vulkan_render_interface& renderer, const char* debug_name, const ri_texture::create_params& params);
    vulkan_ri_texture(vulkan_render_interface& renderer, const char* debug_name, const ri_texture::create_params& params, VkImage image, VkFormat image_format, ri_resource_state common_state);
    virtual ~vulkan_ri_texture();

    result<void> create_resources();

    virtual size_t get_pitch() override;
    virtual size_t get_width() override;
    virtual size_t get_height() override;
    virtual size_t get_depth() override;
    virtual size_t get_mip_levels() override;
    virtual size_t get_dropped_mips() override;

    virtual ri_texture_dimension get_dimensions() const override;
    virtual ri_texture_format get_format() override;

    virtual size_t get_multisample_count() override;

    virtual color get_optimal_clear_color() override;
    virtual float get_optimal_clear_depth() override;
    virtual uint8_t get_optimal_clear_stencil() override;

    virtual bool is_render_target() override;
    virtual bool is_depth_stencil() override;

    virtual bool is_partially_resident() const override;

    virtual size_t get_resident_mips() override;
    virtual void make_mip_resident(size_t mip_index, const std::span<uint8_t>& linear_data) override;
    virtual void make_mip_resident(size_t mip_index, ri_staging_buffer& data_buffer) override;
    virtual void make_mip_non_resident(size_t mip_index) override;
    virtual size_t get_memory_usage_with_residency(size_t mip_count) override;
    virtual bool is_mip_resident(size_t mip_index) override;
    virtual void get_mip_source_data_range(size_t mip_index, size_t& offset, size_t& size) override;
    virtual void begin_mip_residency_change() override;
    virtual void end_mip_residency_change() override;

    virtual ri_resource_state get_initial_state() override;

    // Every VkImage starts life in VK_IMAGE_LAYOUT_UNDEFINED, regardless of what
    // get_initial_state()'s conceptual "common" state is - unlike D3D12 resources, which
    // genuinely start in their creation-specified state. vulkan_ri_command_list::barrier uses
    // this to know whether a texture's first-ever transition needs to come from the real
    // VK_IMAGE_LAYOUT_UNDEFINED rather than get_initial_state()'s steady-state layout.
    bool has_undefined_layout() const;
    void clear_undefined_layout();

    virtual const char* get_debug_name() override;

    virtual void swap(ri_texture* other) override;

    bool calculate_linear_data_mip_range(size_t array_index, size_t mip_index, size_t& offset, size_t& size);

    void add_param_block_reference(vulkan_ri_param_block* block);
    void remove_param_block_reference(vulkan_ri_param_block* block);

public:
    struct mip_residency
    {
        size_t index = 0;

        bool is_resident = false;
        bool is_packed = false;

        // Pixel-space extent of this mip level, used directly as the region for a
        // VkSparseImageMemoryBind (standard mips only - the packed tail is bound as one
        // opaque byte range instead, since it isn't addressable via subresource+offset+extent).
        VkExtent3D pixel_extent = {};

        // Tile-count sizing, used purely for allocate_tiles()/memory accounting.
        size_t tile_count = 0;

        vulkan_ri_tile_manager::allocation tile_allocation;
    };

    vulkan_ri_descriptor_table::allocation get_main_srv() const;
    vulkan_ri_descriptor_table::allocation get_srv(size_t slice, size_t mip) const;
    vulkan_ri_descriptor_table::allocation get_uav(size_t slice, size_t mip) const;

    VkImageView get_rtv_view(size_t slice, size_t mip) const;
    VkImageView get_dsv_view(size_t slice) const;

    VkImageAspectFlags get_aspect_mask() const;

    VkImage get_image();

    const mip_residency& get_mip_residency(size_t index);

    // Byte offset/size of the packed mip tail within the image's opaque sparse memory
    // layout, queried once in create_resources() via VkSparseImageMemoryRequirements.
    VkDeviceSize get_mip_tail_offset() const;
    VkDeviceSize get_mip_tail_size() const;

    size_t calculate_resident_mip_used_bytes();

    size_t get_max_resident_mip();

    void create_views();
    void free_views();
    void recreate_views();

    void calculate_dropped_mips();

private:
    mip_residency* get_first_packed_mip_residency();

    void update_packed_mip_chain_residency();

public:

    vulkan_render_interface& m_renderer;
    std::string m_debug_name;
    ri_texture::create_params m_create_params;

    std::vector<mip_residency> m_mip_residency;

    bool m_packed_mips_resident = false;

    bool m_in_mip_residency_change = false;
    bool m_views_pending_recreate = false;

    memory_type m_memory_type = memory_type::rendering__vram__texture;
    std::unique_ptr<memory_allocation> m_memory_allocation_info = nullptr;

    bool m_owns_image = true;
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;

    VkDeviceSize m_mip_tail_offset = 0;
    VkDeviceSize m_mip_tail_size = 0;

    VkFormat m_format = VK_FORMAT_UNDEFINED;

    ri_descriptor_table m_srv_table = ri_descriptor_table::texture_2d;

    VkImageView m_main_srv_view = VK_NULL_HANDLE;
    vulkan_ri_descriptor_table::allocation m_main_srv;

    std::vector<std::vector<VkImageView>> m_rtv_views;
    std::vector<VkImageView> m_dsv_views;

    std::vector<std::vector<VkImageView>> m_uav_views;
    std::vector<std::vector<vulkan_ri_descriptor_table::allocation>> m_uavs;

    std::vector<std::vector<VkImageView>> m_srv_views;
    std::vector<std::vector<vulkan_ri_descriptor_table::allocation>> m_srvs;

    std::mutex m_reference_mutex;
    std::vector<vulkan_ri_param_block*> m_referencing_param_blocks;

    ri_resource_state m_common_state = ri_resource_state::initial;
    bool m_undefined_layout = true;

    size_t m_pitch = 0;

};

}; // namespace ws
