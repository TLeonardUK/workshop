// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/model/model.h"
#include "workshop.renderer/assets/material/material.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_layout_factory.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"

namespace ws {

model::model(ri_interface& ri_interface, renderer& renderer, asset_manager& ass_manager)
    : m_ri_interface(ri_interface)
    , m_renderer(renderer)
    , m_asset_manager(ass_manager)
{
}

model::~model()
{
}

bool model::post_load()
{
    std::scoped_lock lock(m_mutex);

    for (size_t i = 0; i < materials.size(); i++)
    {
        material_info& mat = materials[i];

        // Load the appropriate material.    
        mat.material = m_asset_manager.request_asset<material>(mat.file.c_str(), 0);
    }

    for (size_t i = 0; i < meshes.size(); i++)
    {
        mesh_info& info = meshes[i];

        // Create index buffer for rendering each mesh.
        std::vector<uint32_t> indices_32;
        std::vector<uint16_t> indices_16;

        ri_buffer::create_params params;
        params.element_count = info.indices.size();
        params.usage = ri_buffer_usage::index_buffer;

        size_t max_index_value = *std::max_element(info.indices.begin(), info.indices.end());

        if (true)//max_index_value >= std::numeric_limits<uint16_t>::max())
        {
            indices_32.reserve(info.indices.size());
            for (size_t index : info.indices)
            {
                indices_32.push_back(static_cast<uint32_t>(index));
            }

            params.element_size = sizeof(uint32_t);
            params.linear_data = std::span{ (uint8_t*)indices_32.data(), indices_32.size() * sizeof(uint32_t) };
        }
        else
        {
            indices_16.reserve(info.indices.size());
            for (size_t index : info.indices)
            {
                indices_16.push_back(static_cast<uint16_t>(index));
            }

            params.element_size = sizeof(uint16_t);
            params.linear_data = std::span{ (uint8_t*)indices_16.data(), indices_16.size() * sizeof(uint16_t) };
        }

        std::string index_buffer_name = string_format("Model Index Buffer[%zi]: %s", i, name.c_str());
        info.index_buffer = m_renderer.get_render_interface().create_buffer(params, index_buffer_name.c_str());
    }
    
    return true;
}

ri_param_block* model::find_or_create_param_block(const char* type, size_t key, param_block_setup_callback_t setup_callback)
{
    std::scoped_lock lock(m_mutex);

    size_t hash = 0;
    hash_combine(hash, type);
    hash_combine(hash, key);

    if (auto iter = m_param_blocks.find(hash); iter != m_param_blocks.end())
    {
        return iter->second.get();
    }

    std::unique_ptr<ri_param_block> new_block = m_renderer.get_param_block_manager().create_param_block(type);
    setup_callback(*new_block.get());

    ri_param_block* new_block_ptr = new_block.get();
    m_param_blocks[hash] = std::move(new_block);

    return new_block_ptr;
}

model::vertex_buffer* model::find_or_create_vertex_buffer(const ri_data_layout& layout)
{
    std::scoped_lock lock(m_mutex);

    if (auto iter = m_vertex_buffers.find(layout); iter != m_vertex_buffers.end())
    {
        return iter->second.get();
    }

    std::unique_ptr<ri_layout_factory> factory = m_renderer.get_render_interface().create_layout_factory(layout, ri_layout_usage::buffer);
    for (geometry_vertex_stream& stream : geometry->get_vertex_streams())
    {
        bool is_needed = false;
        for (const ri_data_layout::field& field : layout.fields)
        {
            if (field.name == stream.name)
            {
                is_needed = true;
                break;
            }
        }

        if (is_needed)
        {
            factory->add(stream.name.c_str(), stream.data, stream.element_size, ri_convert_geometry_data_type(stream.type));
        }
    }

    std::string vertex_buffer_name = string_format("Model Vertex Buffer[%zi]: %s", m_vertex_buffers.size(), name.c_str());

    std::unique_ptr<vertex_buffer> buf = std::make_unique<vertex_buffer>();
    buf->vertex_buffer = factory->create_vertex_buffer(vertex_buffer_name.c_str());;

    vertex_buffer* new_buf_ptr = buf.get();
    m_vertex_buffers[layout] = std::move(buf);

    return new_buf_ptr;
}

void model::swap(model* other)
{
    std::scoped_lock lock(m_mutex);

    std::swap(materials, other->materials);
    std::swap(meshes, other->meshes);
    std::swap(geometry, other->geometry);

    // Cleared cached data as these point to old data.
    m_param_blocks.clear();
    m_vertex_buffers.clear();
}

}; // namespace ws
