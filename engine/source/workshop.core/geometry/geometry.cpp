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

void geometry::add_vertex_stream(const char* field_name, const std::vector<uint8_t>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector2b>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector3b>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector4b>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<int32_t>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector2i>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector3i>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector4i>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<uint32_t>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector2u>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector3u>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector4u>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<float>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector2>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector3>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector4>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<matrix2>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<matrix3>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<matrix4>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<double>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector2d>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector3d>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<vector4d>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<matrix2d>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<matrix3d>& values)
{
}

void geometry::add_vertex_stream(const char* field_name, const std::vector<matrix4d>& values)
{
}

void geometry::add_material(const char* name, const std::vector<size_t>& indices)
{
}

size_t geometry::get_vertex_count()
{
    return 0;
}

const std::vector<geometry_vertex_stream>& geometry::get_vertex_streams()
{
    return {};
}

const std::vector<geometry_material>& geometry::get_materials()
{
    return {};
}

std::unique_ptr<geometry> geometry::load(const char* path)
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
    if (_stricmp(extension.c_str(), ".obj") == 0 ||
        _stricmp(extension.c_str(), ".fbx") == 0)
    {
        result = geometry_assimp_loader::load(buffer);
    }
    else
    {
        db_error(asset, "Failed to determine file format when loading geometry: %s", path);
        return nullptr;
    }

    return result;
}

}; // namespace workshop
