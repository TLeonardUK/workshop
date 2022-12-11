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
    dx12_ri_layout_factory(dx12_render_interface& renderer, ri_data_layout layout);
    virtual ~dx12_ri_layout_factory();

    virtual void clear() override;

    virtual void add(const char* field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type) override;

    virtual std::unique_ptr<ri_buffer> create_vertex_buffer(const char* name) override;
    virtual std::unique_ptr<ri_buffer> create_index_buffer(const char* name, const std::vector<uint16_t>& indices) override;
    virtual std::unique_ptr<ri_buffer> create_index_buffer(const char* name, const std::vector<uint32_t>& indices) override;

private:
    void validate();

private:
    struct field
    {
        std::string name;
        ri_data_type type;
        size_t size;
        size_t offset;
        bool added = false;
    };

    std::unordered_map<std::string, field> m_fields;

    dx12_render_interface& m_renderer;
    ri_data_layout m_layout;

    size_t m_element_size;
    size_t m_element_count;

    std::vector<uint8_t> m_buffer;

};

}; // namespace ws
