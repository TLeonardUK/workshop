// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/drawing/color.h"

#include "workshop.core/math/vector2.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/vector4.h"
#include "workshop.core/math/matrix2.h"
#include "workshop.core/math/matrix3.h"
#include "workshop.core/math/matrix4.h"
#include "workshop.core/math/aabb.h"

#include <vector>
#include <span>

namespace ws {

//  Data types that can exist inside a geometry stream.
enum class geometry_data_type
{
	t_bool,
	t_int,
	t_uint,
	t_half,
	t_float,
	t_double,

	t_bool2,
	t_int2,
	t_uint2,
	t_half2,
	t_float2,
	t_double2,

	t_bool3,
	t_int3,
	t_uint3,
	t_half3,
	t_float3,
	t_double3,

	t_bool4,
	t_int4,
	t_uint4,
	t_half4,
	t_float4,
	t_double4,

	t_float2x2,
	t_double2x2,
	t_float3x3,
	t_double3x3,
	t_float4x4,
	t_double4x4,

	COUNT
};

// Represents an individual vertex stream held in a geometry instance.
struct geometry_vertex_stream
{
	// Name of this stream.
	std::string name;

	// Data type of elements stored in this stream.
	geometry_data_type type;

	// How large an individual element is in the stream. This can be derived from type.
	size_t element_size;

	// Buffer containing the vertex stream data, reinterpret this to the expected type.
	std::vector<uint8_t> data;
};

// Represents an individual material held in a geometry instance.
struct geometry_material
{
	// Name of this material.
	std::string name;

    // Index of this material for easy lookup.
    size_t index;
};

// Represents an individual mesh held in a geometry instance.
struct geometry_mesh
{
    // Name of this mesh if one is available.
    std::string name;

    // The indice of the material to draw this mesh with.
    size_t material_index;

    // Indices to draw for this mesh.
    std::vector<size_t> indices;

    // Bounds of the vertices that contribute to this mesh.
    aabb bounds;
};

// ================================================================================================
//  Represents a set of geometry information, vertices, indices and material references.
//  
//  This is akin to the pixmap class but for geometry information.
// 
//  This class is not intended for use at runtime, geometry at runtime should be cooked in
//  a renderer appropriate format. This is instead mostly meant for manipulation at cook time.
// ================================================================================================
class geometry
{
public:

    // Adds a stream of vertex data of the given type. All streams added are 
    // expected to be of the same length.
	
	// bool
	void add_vertex_stream(const char* field_name, const std::vector<uint8_t>& values); // Fucking bools.
	void add_vertex_stream(const char* field_name, const std::vector<vector2b>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector3b>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector4b>& values);

	// int
	void add_vertex_stream(const char* field_name, const std::vector<int32_t>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector2i>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector3i>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector4i>& values);

	// uint
	void add_vertex_stream(const char* field_name, const std::vector<uint32_t>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector2u>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector3u>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector4u>& values);

	// float
	void add_vertex_stream(const char* field_name, const std::vector<float>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector2>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector3>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector4>& values);
	void add_vertex_stream(const char* field_name, const std::vector<matrix2>& values);
	void add_vertex_stream(const char* field_name, const std::vector<matrix3>& values);
	void add_vertex_stream(const char* field_name, const std::vector<matrix4>& values);

	// double
	void add_vertex_stream(const char* field_name, const std::vector<double>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector2d>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector3d>& values);
	void add_vertex_stream(const char* field_name, const std::vector<vector4d>& values);
	void add_vertex_stream(const char* field_name, const std::vector<matrix2d>& values);
	void add_vertex_stream(const char* field_name, const std::vector<matrix3d>& values);
	void add_vertex_stream(const char* field_name, const std::vector<matrix4d>& values);

    // Adds a new material that will be used to render the given set of vertices.
    // Returns the index of the material for use with add_mesh.
    size_t add_material(const char* name);

    // Adds a new mesh that will render the given set of vertices.
    // Returns the index of this mesh.
    size_t add_mesh(const char* name, size_t material_index, const std::vector<size_t>& indices, const aabb& bounds);

    // How many vertices exist in the geometry.
    size_t get_vertex_count();

	// Gets all the vertex streams in this geometry.
	std::vector<geometry_vertex_stream>& get_vertex_streams();

    // Gets all the meshes in this geometry.
    std::vector<geometry_mesh>& get_meshes();

	// Gets all the materials in this geometry.
	std::vector<geometry_material>& get_materials();

	// Attempts to get a material with the given name.
	geometry_material* get_material(const char* name);

    // Attempts to load the geometry data from the given file.
    // Returns nullptr if not able to load or attempting to load an unsupported format.
    static std::unique_ptr<geometry> load(const char* path, const vector3& scale = vector3::one);

public:

	// Bounds of all vertices in the mesh.
	aabb bounds;

private:
	void add_vertex_stream(const char* field_name, std::span<uint8_t> data, size_t element_size, geometry_data_type type);

private:
	std::vector<geometry_vertex_stream> m_streams;
	std::vector<geometry_material> m_materials;
    std::vector<geometry_mesh> m_meshes;

};

}; // namespace workshop
