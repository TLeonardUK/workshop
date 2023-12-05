// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/geometry/geometry.h"
#include "workshop.core/geometry/geometry_assimp_loader.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/math/math.h"

namespace ws {

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<uint8_t>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(uint8_t)), sizeof(uint8_t), geometry_data_type::t_bool);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector2b>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector2b)), sizeof(vector2b), geometry_data_type::t_bool2);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector3b>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector3b)), sizeof(vector3b), geometry_data_type::t_bool3);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector4b>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector4b)), sizeof(vector4b), geometry_data_type::t_bool4);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<int32_t>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(int32_t)), sizeof(int32_t), geometry_data_type::t_int);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector2i>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector2i)), sizeof(vector2i), geometry_data_type::t_int2);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector3i>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector3i)), sizeof(vector3i), geometry_data_type::t_int3);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector4i>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector4i)), sizeof(vector4i), geometry_data_type::t_int4);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<uint32_t>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(uint32_t)), sizeof(uint32_t), geometry_data_type::t_uint);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector2u>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector2u)), sizeof(vector2u), geometry_data_type::t_uint2);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector3u>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector3u)), sizeof(vector3u), geometry_data_type::t_uint3);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector4u>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector4u)), sizeof(vector4u), geometry_data_type::t_uint4);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<float>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(float)), sizeof(float), geometry_data_type::t_float);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector2>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector2)), sizeof(vector2), geometry_data_type::t_float2);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector3>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector3)), sizeof(vector3), geometry_data_type::t_float3);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector4>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector4)), sizeof(vector4), geometry_data_type::t_float4);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<matrix2>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix2)), sizeof(matrix2), geometry_data_type::t_float2x2);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<matrix3>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix3)), sizeof(matrix3), geometry_data_type::t_float3x3);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<matrix4>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix4)), sizeof(matrix4), geometry_data_type::t_float4x4);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<double>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(double)), sizeof(double), geometry_data_type::t_double);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector2d>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector2d)), sizeof(vector2d), geometry_data_type::t_double2);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector3d>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector3d)), sizeof(vector3d), geometry_data_type::t_double3);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<vector4d>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(vector4d)), sizeof(vector4d), geometry_data_type::t_double4);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<matrix2d>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix2d)), sizeof(matrix2d), geometry_data_type::t_double2x2);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<matrix3d>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix3d)), sizeof(matrix3d), geometry_data_type::t_double3x3);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, const std::vector<matrix4d>& values)
{
    add_vertex_stream(type, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix4d)), sizeof(matrix4d), geometry_data_type::t_double4x4);
}

void geometry::add_vertex_stream(geometry_vertex_stream_type type, std::span<uint8_t> data, size_t element_size, geometry_data_type data_type)
{
    geometry_vertex_stream& stream = m_streams.emplace_back();
    stream.type = type;
    stream.data_type = data_type;
    stream.data.insert(stream.data.end(), data.data(), data.data() + data.size_bytes());
    stream.element_size = element_size;
}

size_t geometry::add_material(const char* name)
{
    geometry_material& mat = m_materials.emplace_back();
    mat.name = name;
    mat.index = m_materials.size() - 1;

    return m_materials.size() - 1;
}

size_t geometry::add_mesh(const char* name, size_t material_index, const std::vector<uint32_t>& indices, const aabb& bounds, float min_texel_area, float max_texel_area, float avg_texel_area, float min_world_area, float max_world_area, float avg_world_area)
{
    geometry_mesh& mat = m_meshes.emplace_back();
    mat.name = name;
    mat.indices = indices;
    mat.bounds = bounds;
    mat.material_index = material_index;

    mat.min_texel_area = min_texel_area;
    mat.max_texel_area = max_texel_area;
    mat.avg_texel_area = avg_texel_area;
    mat.min_world_area = min_world_area;
    mat.max_world_area = max_world_area;
    mat.avg_world_area = avg_world_area;

    return m_meshes.size() - 1;
}

size_t geometry::get_vertex_count()
{
    if (m_streams.size() == 0)
    {
        return 0;
    }

    geometry_vertex_stream& stream = m_streams[0];
    return stream.data.size() / stream.element_size;
}

std::vector<geometry_vertex_stream>& geometry::get_vertex_streams()
{
    return m_streams;
}

geometry_vertex_stream* geometry::find_vertex_stream(geometry_vertex_stream_type type)
{
    for (geometry_vertex_stream& stream : m_streams)
    {
        if (stream.type == type)
        {
            return &stream;
        }
    }
    return nullptr;
}

void geometry::clear_vertex_stream_data(geometry_vertex_stream_type type)
{
    for (geometry_vertex_stream& stream : m_streams)
    {
        if (stream.type == type)
        {
            stream.data.clear();
            stream.data.shrink_to_fit();
        }
    }
}

std::vector<geometry_material>& geometry::get_materials()
{
    return m_materials;
}

std::vector<geometry_mesh>& geometry::get_meshes()
{
    return m_meshes;
}

geometry_material* geometry::get_material(const char* name)
{
    for (geometry_material& mat : m_materials)
    {
        if (mat.name == name)
        {
            return &mat;
        }
    }
    return nullptr;
}

std::unique_ptr<geometry> geometry::load(const char* path, const vector3& scale, bool high_quality)
{
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, false);
    if (!stream)
    {
        db_error(asset, "Failed to open stream when loading geometry: %s", path);
        return nullptr;
    }

    std::vector<char> buffer;
    buffer.resize(stream->length());
    if (stream->read(buffer.data(), buffer.size()) != buffer.size())
    {
        db_error(asset, "Failed to read full file when loading geometry: %s", path);
        return nullptr;
    }

    std::unique_ptr<geometry> result;

    std::string extension = virtual_file_system::get_extension(path);
    if (geometry_assimp_loader::supports_extension(extension.c_str()))
    {
        result = geometry_assimp_loader::load(buffer, path, scale, high_quality);
    }
    else
    {
        db_error(asset, "Failed to determine file format when loading geometry: %s", path);
        return nullptr;
    }

    return result;
}

}; // namespace workshop
