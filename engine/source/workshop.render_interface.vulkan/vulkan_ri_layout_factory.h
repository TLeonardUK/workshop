// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_layout_factory.h"
#include "workshop.render_interface/ri_types.h"
#include "workshop.core/utils/result.h"
#include "workshop.core/hashing/string_hash.h"

#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace ws {

class vulkan_render_interface;

// ================================================================================================
//  Handles generating buffers in a layout consumable by the gpu.
// ================================================================================================
class vulkan_ri_layout_factory : public ri_layout_factory
{
public:
    vulkan_ri_layout_factory(vulkan_render_interface& renderer, ri_data_layout layout, ri_layout_usage usage);
    virtual ~vulkan_ri_layout_factory();

    virtual void clear() override;
    virtual size_t get_instance_size() override;

    virtual void add(string_hash field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type) override;

    virtual std::unique_ptr<ri_buffer> create_vertex_buffer(const char* name) override;
    virtual std::unique_ptr<ri_buffer> create_index_buffer(const char* name, const std::vector<uint32_t>& indices) override;

public:
    struct field
    {
        string_hash name_hash;
        ri_data_type type;
        size_t size;
        size_t offset;
        size_t index;
        bool added = false;
    };

    size_t get_field_count();
    field get_field(size_t index);
    bool get_field_info(string_hash name, field& info);

    void transpose_matrices(void* field, ri_data_type type);

private:
    void validate();

private:
    std::unordered_map<string_hash, field> m_fields;

    vulkan_render_interface& m_renderer;
    ri_data_layout m_layout;
    ri_layout_usage m_usage;

    size_t m_element_size = 0;
    size_t m_element_count = 0;

    std::vector<uint8_t> m_buffer;

};

}; // namespace ws
