// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_param_block.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include "workshop.render_interface.dx12/dx12_ri_param_block_archetype.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"

#include <array>
#include <string>
#include <unordered_map>

namespace ws {

class engine;
class dx12_render_interface;
class dx12_ri_texture;

// ================================================================================================
//  Implementation of a parameter block (aka constant buffer) for dx12.
// ================================================================================================
class dx12_ri_param_block : public ri_param_block
{
public:
    dx12_ri_param_block(dx12_render_interface& renderer, dx12_ri_param_block_archetype& archetype);
    virtual ~dx12_ri_param_block();

    virtual bool set(string_hash field_name, const ri_texture& resource) override;
    virtual bool set(string_hash field_name, const ri_sampler& resource) override;
    virtual bool set(string_hash field_name, const ri_texture_view& resource, bool writable) override;
    virtual bool set(string_hash field_name, const ri_buffer& resource, bool writable) override;
    virtual bool set(string_hash field_name, const ri_raytracing_tlas& resource) override;

    virtual bool clear_buffer(string_hash field_name) override;

    virtual ri_param_block_archetype* get_archetype() override;

    virtual void get_table(size_t& index, size_t& offset) override;

public:
    // Gets the gpu address and also increments the use count
    // for the param block, the next set will cause it to mutate.
    void* consume();

    // Called by renderer to upload the state of a dirty param block.
    void upload_state();

    // Called when a texture is deleted or in some other way removed and the refernces we hold to it
    // need to be cleared.
    void clear_texture_references(dx12_ri_texture* texture);

    // Called when a texture that we hold a reference to has been modified and we need to update any srvs/etc
    // that we currently use.
    void referenced_texture_modified(dx12_ri_texture* texture);

private:
    void mark_dirty();

    virtual bool set(string_hash field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type) override;
    bool set(size_t field_index, const std::span<uint8_t>& values, size_t value_size, ri_data_type type);

    bool set(size_t field_index, const ri_texture_view& resource, bool writable, bool do_not_add_references);

    void transpose_matrices(void* field, ri_data_type type);

    void add_texture_reference(size_t field_index, const ri_texture_view& view, bool writable);

private:
    dx12_render_interface& m_renderer;
    dx12_ri_param_block_archetype& m_archetype;

    std::atomic_bool m_cpu_dirty = false;
    std::vector<uint8_t> m_cpu_shadow_data;

    size_t m_use_count = 0;
    size_t m_last_mutate_use_count = std::numeric_limits<size_t>::max();

    dx12_ri_param_block_archetype::allocation m_allocation;
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
