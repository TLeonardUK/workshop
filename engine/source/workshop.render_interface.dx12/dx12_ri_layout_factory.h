// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_layout_factory.h"
#include "workshop.render_interface/ri_types.h"
#include "workshop.core/utils/result.h"

#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>
#include <span>

namespace ws {

class dx12_render_interface;

// ================================================================================================
//  Handles generating buffers in a layout consumable by the gpu.
// ================================================================================================
class dx12_ri_layout_factory 
    : public ri_layout_factory
{
public:
    dx12_ri_layout_factory(dx12_render_interface& renderer, ri_data_layout layout, ri_layout_usage usage);
    virtual ~dx12_ri_layout_factory();

    virtual void clear() override;
    virtual size_t get_instance_size() override;

    virtual void add(const char* field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type) override;

    virtual std::unique_ptr<ri_buffer> create_vertex_buffer(const char* name) override;
    #if 0
    virtual std::unique_ptr<ri_buffer> create_index_buffer(const char* name, const std::vector<uint16_t>& indices) override;
    #endif
    virtual std::unique_ptr<ri_buffer> create_index_buffer(const char* name, const std::vector<uint32_t>& indices) override;

public:

    struct field
    {
        std::string name;
        ri_data_type type;
        size_t size;
        size_t offset;
        size_t index;
        bool added = false;
    };

    size_t get_field_count();
    field get_field(size_t index);
    bool get_field_info(const char* name, field& info);

    void transpose_matrices(void* field, ri_data_type type);

private:
    void validate();

private:

    std::unordered_map<std::string, field> m_fields;

    dx12_render_interface& m_renderer;
    ri_data_layout m_layout;
    ri_layout_usage m_usage;

    size_t m_element_size;
    size_t m_element_count;

    std::vector<uint8_t> m_buffer;

};

}; // namespace ws
