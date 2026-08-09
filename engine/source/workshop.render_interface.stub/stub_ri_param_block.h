// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_param_block.h"

namespace ws {

// ================================================================================================
//  Stub implementation of a parameter block (aka constant buffer), performs no actual work.
// ================================================================================================
class stub_ri_param_block : public ri_param_block
{
public:
    stub_ri_param_block(ri_param_block_archetype* archetype);

    virtual bool set(string_hash field_name, const ri_texture& resource) override;
    virtual bool set(string_hash field_name, const ri_texture_view& resource, bool writable = false) override;
    virtual bool set(string_hash field_name, const ri_sampler& resource) override;
    virtual bool set(string_hash field_name, const ri_buffer& resource, bool writable = false) override;
    virtual bool set(string_hash field_name, const ri_raytracing_tlas& resource) override;

    virtual bool clear_buffer(string_hash field_name) override;

    virtual ri_param_block_archetype* get_archetype() override;

    virtual void get_table(size_t& index, size_t& offset) override;

private:
    virtual bool set(string_hash field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type) override;

private:
    ri_param_block_archetype* m_archetype;

};

}; // namespace ws
