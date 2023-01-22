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

#include <vector>

namespace ws {

// Represents the type of data of each element that makes up a geometry vertex stream.
enum class geometry_vertex_stream_type
{

};

// Represents an individual vertex stream held in a geometry instance.
struct geometry_vertex_stream
{
	// Name of this stream.
	std::string name;

	// Type of data held in this stream.
	geometry_vertex_stream_type type;

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

	// Indices this material draws.
	std::vector<size_t> indices;
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
    void add_material(const char* name, const std::vector<size_t>& indices);

    // How many vertices exist in the geometry.
    size_t get_vertex_count();

	// Gets all the vertex streams in this geometry.
	const std::vector<geometry_vertex_stream>& get_vertex_streams();

	// Gets all the materials in this geometry.
	const std::vector<geometry_material>& get_materials();

    // Attempts to load the geometry data from the given file.
    // Returns nullptr if not able to load or attempting to load an unsupported format.
    static std::unique_ptr<geometry> load(const char* path);

private:
	std::vector<geometry_vertex_stream> m_streams;
	std::vector<geometry_material> m_materials;

};

}; // namespace workshop
