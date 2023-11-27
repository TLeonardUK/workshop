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

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Implementation of a parameter block (aka constant buffer) for dx12.
// ================================================================================================
class dx12_ri_param_block : public ri_param_block
{
public:
    dx12_ri_param_block(dx12_render_interface& renderer, dx12_ri_param_block_archetype& archetype);
    virtual ~dx12_ri_param_block();

    virtual bool set(const char* field_name, const ri_texture& resource) override;
    virtual bool set(const char* field_name, const ri_sampler& resource) override;
    virtual bool set(const char* field_name, const ri_texture_view& resource, bool writable) override;
    virtual bool set(const char* field_name, const ri_buffer& resource, bool writable) override;
    virtual bool set(const char* field_name, const ri_raytracing_tlas& resource) override;

    virtual bool clear_buffer(const char* field_name) override;

    virtual ri_param_block_archetype* get_archetype() override;

    virtual void get_table(size_t& index, size_t& offset) override;

public:
    // Gets the gpu address and also increments the use count
    // for the param block, the next set will cause it to mutate.
    void* consume();

private:
    void mutate();

    virtual bool set(const char* field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type) override;

    void transpose_matrices(void* field, ri_data_type type);

private:
    dx12_render_interface& m_renderer;
    dx12_ri_param_block_archetype& m_archetype;

    size_t m_use_count = 0;
    size_t m_last_mutate_use_count = std::numeric_limits<size_t>::max();

    dx12_ri_param_block_archetype::allocation m_allocation;

    std::vector<bool> m_fields_set;

    std::recursive_mutex m_consume_mutex;

};

}; // namespace ws
