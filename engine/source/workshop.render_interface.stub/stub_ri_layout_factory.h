// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_layout_factory.h"

#include <memory>
#include <vector>

namespace ws {

// ================================================================================================
//  Stub implementation for generating buffers in a layout consumable by the gpu.
// ================================================================================================
class stub_ri_layout_factory : public ri_layout_factory
{
public:
    stub_ri_layout_factory(ri_data_layout layout, ri_layout_usage usage);

    virtual void clear() override;
    virtual size_t get_instance_size() override;

    virtual std::unique_ptr<ri_buffer> create_vertex_buffer(const char* name) override;
    virtual std::unique_ptr<ri_buffer> create_index_buffer(const char* name, const std::vector<uint32_t>& indices) override;

    virtual void add(string_hash field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type) override;

private:
    ri_data_layout m_layout;
    ri_layout_usage m_usage;
    size_t m_instance_size = 0;

};

}; // namespace ws
