// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/geometry/geometry.h"
#include "workshop.core/math/aabb.h"

#include "workshop.render_interface/ri_types.h"
#include "workshop.render_interface/ri_raytracing_blas.h"

#include "workshop.renderer/assets/material/material.h"

#include "workshop.renderer/renderer.h"
#include <array>
#include <unordered_map>

namespace ws {

class asset;
class ri_interface;
class ri_buffer;
class ri_param_block;
class renderer;
class asset_manager;

// ================================================================================================
//  Model assets respresent all the vertex/index/material references required to render
//  a mesh to the scene.
// ================================================================================================
class model : public asset
{
public:
    struct material_info
    {
        std::string name;
        std::string file;

        asset_ptr<material> material;
    };

    struct mesh_info
    {
        std::string name;
        std::vector<uint32_t> indices;
        std::unique_ptr<ri_buffer> index_buffer;
        std::unique_ptr<ri_raytracing_blas> blas;

        size_t material_index;
        float min_texel_area;
        float max_texel_area;
        float min_world_area;
        float max_world_area;
        aabb bounds;
    };

    struct vertex_buffer
    {
        std::unique_ptr<ri_buffer> vertex_buffer;
    };

    // If you modify these, ensure you update model_info in common.yaml
    inline static ri_data_type k_vertex_stream_runtime_types[static_cast<int>(geometry_vertex_stream_type::COUNT)] = {
        ri_data_type::t_float3,
        ri_data_type::t_compressed_unit_vector,
        ri_data_type::t_compressed_unit_vector,
        ri_data_type::t_compressed_unit_vector,
        ri_data_type::t_float2,
        ri_data_type::t_float2,
        ri_data_type::t_float2,
        ri_data_type::t_float2,
        ri_data_type::t_float4,
        ri_data_type::t_float4,
        ri_data_type::t_float4,
        ri_data_type::t_float4
    };

public:
    using param_block_setup_callback_t = std::function<void(ri_param_block& block)>;

    model(ri_interface& ri_interface, renderer& renderer, asset_manager& ass_manager);
    virtual ~model();

    // Finds a previously created bottom level acceleration structure for the given mesh index.
    // If none has previously been created, one will be created.
    ri_raytracing_blas* find_or_create_blas(size_t mesh_index);

    // Finds a previous created param block of the given type and key, or if none has been created
    // makes a new one and calls the setup_callback function.
    ri_param_block* find_or_create_param_block(const char* type, size_t key, param_block_setup_callback_t setup_callback);

    // Finds the buffer for the given vertex stream.
    vertex_buffer* find_vertex_stream_buffer(geometry_vertex_stream_type stream_type);

    // Gets the param block that describes where all the vertex stream buffers are that
    // make up the geometry for this model.
    ri_param_block& get_model_info_param_block(size_t mesh_index);

    void swap(model* other);

public:
    std::vector<material_info> materials;
    std::vector<mesh_info> meshes;
    std::unique_ptr<geometry> geometry;
    std::string source_node;

protected:
    virtual bool load_dependencies() override;

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;
    asset_manager& m_asset_manager;

    std::unordered_map<size_t, std::unique_ptr<ri_param_block>> m_param_blocks;
    std::unordered_map<ri_data_layout, std::unique_ptr<vertex_buffer>> m_vertex_buffers;

    std::array<std::unique_ptr<vertex_buffer>, (int)geometry_vertex_stream_type::COUNT> m_vertex_streams;

    std::vector<std::unique_ptr<ri_param_block>> m_model_info_param_blocks;
    

    std::mutex m_mutex;

};

}; // namespace workshop
