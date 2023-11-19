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

bool model::load_dependencies()
{
    std::scoped_lock lock(m_mutex);

    for (size_t i = 0; i < materials.size(); i++)
    {
        material_info& mat = materials[i];

        // Load the appropriate material.    
        mat.material = m_asset_manager.request_asset<material>(mat.file.c_str(), 0);
    }

    // Create index buffer for each sub-mesh.
    for (size_t i = 0; i < meshes.size(); i++)
    {
        mesh_info& info = meshes[i];

        // Create index buffer for rendering each mesh.
        std::vector<uint32_t> indices_32;
#if 0
        std::vector<uint16_t> indices_16;
#endif

        ri_buffer::create_params params;
        params.element_count = info.indices.size();
        params.usage = ri_buffer_usage::index_buffer;

        size_t max_index_value = *std::max_element(info.indices.begin(), info.indices.end());

        // Note: Before you enable 16bit index buffers, check out the shaders that read from the index buffer
        // indirectly (eg. the raytracing ones). They don't currently support loading 16bit values.
#if 0
        if (max_index_value >= std::numeric_limits<uint16_t>::max())
        {
#endif
            indices_32.reserve(info.indices.size());
            for (uint32_t index : info.indices)
            {
                indices_32.push_back(static_cast<uint32_t>(index));
            }

            params.element_size = sizeof(uint32_t);
            params.linear_data = std::span{ (uint8_t*)indices_32.data(), indices_32.size() * sizeof(uint32_t) };
#if 0
        }
        else
        {
            indices_16.reserve(info.indices.size());
            for (uint32_t index : info.indices)
            {
                indices_16.push_back(static_cast<uint16_t>(index));
            }

            params.element_size = sizeof(uint16_t);
            params.linear_data = std::span{ (uint8_t*)indices_16.data(), indices_16.size() * sizeof(uint16_t) };
        }
#endif

        std::string index_buffer_name = string_format("Model Index Buffer[%zi]: %s", i, name.c_str());
        info.index_buffer = m_renderer.get_render_interface().create_buffer(params, index_buffer_name.c_str());

        // Create a model_info param block that points to all the vertex stream buffers.
        std::unique_ptr<ri_param_block> model_info = m_renderer.get_param_block_manager().create_param_block("model_info");
        model_info->set("index_buffer", *info.index_buffer, false);
        model_info->set("index_size", (int)params.element_size);
        m_model_info_param_blocks.push_back(std::move(model_info));
    }

    // Create buffer for each vertex stream.
    for (size_t i = 0; i < (int)geometry_vertex_stream_type::COUNT; i++)
    {
        geometry_vertex_stream_type stream_type = (geometry_vertex_stream_type)i;

        const char* stream_name = geometry_vertex_stream_type_strings[i];
        std::string field_name = string_format("%s_buffer", stream_name);

        geometry_vertex_stream* stream = geometry->find_vertex_stream(stream_type);
        if (!stream)
        {
            for (auto& param_block : m_model_info_param_blocks)
            {
                param_block->clear_buffer(field_name.c_str());
            }
            m_vertex_streams[i] = nullptr;
            continue;
        }

        ri_data_layout stream_layout;
        stream_layout.fields.push_back({ stream_name, k_vertex_stream_runtime_types[i] });

        std::unique_ptr<ri_layout_factory> factory = m_renderer.get_render_interface().create_layout_factory(stream_layout, ri_layout_usage::buffer);
        factory->add(stream_name, stream->data, stream->element_size, ri_convert_geometry_data_type(stream->data_type));

        std::string vertex_buffer_name = string_format("Model Vertex Stream[%s]: %s", stream_name, name.c_str());

        std::unique_ptr<vertex_buffer> buf = std::make_unique<vertex_buffer>();
        buf->vertex_buffer = factory->create_vertex_buffer(vertex_buffer_name.c_str());;

        for (auto& param_block : m_model_info_param_blocks)
        {
            param_block->set(field_name.c_str(), *buf->vertex_buffer, false);
        }
        m_vertex_streams[i] = std::move(buf);

        // We can clear out the cpu information for all streams except position (we use position for picking).
        if (stream_type != geometry_vertex_stream_type::position)
        {
            geometry->clear_vertex_stream_data(stream_type);
        }
    }

    return true;
}

ri_raytracing_blas* model::find_or_create_blas(size_t mesh_index)
{
    model::vertex_buffer* buffer = nullptr;

    {
        buffer = find_vertex_stream_buffer(geometry_vertex_stream_type::position);
    }

    {
        std::scoped_lock lock(m_mutex);

        mesh_info& info = meshes[mesh_index];
        if (info.blas)
        {
            return info.blas.get();
        }


        std::string blas_name = string_format("Model BLAS[%zi]: %s", mesh_index, name.c_str());
        info.blas = m_renderer.get_render_interface().create_raytracing_blas(blas_name.c_str());
        info.blas->update(buffer->vertex_buffer.get(), info.index_buffer.get());
    
        return info.blas.get();
    }
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

model::vertex_buffer* model::find_vertex_stream_buffer(geometry_vertex_stream_type stream_type)
{
    std::scoped_lock lock(m_mutex);

    return m_vertex_streams[(int)stream_type].get();
}

ri_param_block& model::get_model_info_param_block(size_t mesh_index)
{
    return *m_model_info_param_blocks[mesh_index];
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
