// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_param_block.h"
#include "workshop.render_interface.vulkan/vulkan_ri_param_block_archetype.h"
#include "workshop.core/utils/result.h"

#include <array>
#include <mutex>
#include <string>
#include <unordered_map>

namespace ws {

class vulkan_render_interface;
class vulkan_ri_texture;

// ================================================================================================
//  Implementation of a parameter block (aka constant buffer) for vulkan.
// ================================================================================================
class vulkan_ri_param_block : public ri_param_block
{
public:
    vulkan_ri_param_block(vulkan_render_interface& renderer, vulkan_ri_param_block_archetype& archetype);
    virtual ~vulkan_ri_param_block();

    virtual bool set(string_hash field_name, const ri_texture& resource) override;
    virtual bool set(string_hash field_name, const ri_texture_view& resource, bool writable = false) override;
    virtual bool set(string_hash field_name, const ri_sampler& resource) override;
    virtual bool set(string_hash field_name, const ri_buffer& resource, bool writable = false) override;
    virtual bool set(string_hash field_name, const ri_raytracing_tlas& resource) override;

    virtual bool clear_buffer(string_hash field_name) override;

    virtual ri_param_block_archetype* get_archetype() override;

    virtual void get_table(size_t& index, size_t& offset) override;

public:
    // Gets the gpu address of this param block's allocation, for binding as a push constant.
    // Also warns about any fields that were never set.
    VkDeviceAddress consume();

    // Uploads the cpu-shadow copy of this param block's data to the gpu. Called by the
    // renderer whenever this block has been marked dirty and uploads are flushed.
    void upload_state();

    // Called by vulkan_ri_texture when a texture this block references is destroyed, or has
    // had its views recreated (eg. due to a mip residency change) and needs the bindless
    // index this block stores for it refreshed.
    void clear_texture_references(vulkan_ri_texture* texture);
    void referenced_texture_modified(vulkan_ri_texture* texture);

private:
    void mark_dirty();

    virtual bool set(string_hash field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type) override;
    bool set(size_t field_index, const std::span<uint8_t>& values, size_t value_size, ri_data_type type);

    bool set(size_t field_index, const ri_texture_view& resource, bool writable, bool do_not_add_references);

    void add_texture_reference(size_t field_index, const ri_texture_view& view, bool writable);

private:
    vulkan_render_interface& m_renderer;
    vulkan_ri_param_block_archetype& m_archetype;

    std::atomic_bool m_cpu_dirty = false;
    std::vector<uint8_t> m_cpu_shadow_data;

    vulkan_ri_param_block_archetype::allocation m_allocation;
    std::vector<bool> m_fields_set;

    struct referenced_texture
    {
        ri_texture_view view;
        bool writable;
    };

    std::mutex m_reference_mutex;
    std::unordered_map<size_t, referenced_texture> m_referenced_textures;

};

}; // namespace ws
