// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"

#include "workshop.core/math/vector2.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/vector4.h"
#include "workshop.core/math/matrix2.h"
#include "workshop.core/math/matrix3.h"
#include "workshop.core/math/matrix4.h"

#include <span>

namespace ws {

class ri_buffer;

// ================================================================================================
//  Represents a factory that create data buffers in a format consumable by the gpu.
// ================================================================================================
class ri_layout_factory
{
public:

	virtual void clear() = 0;

	// bool
	void add(const char* field_name, const std::vector<uint8_t>& values); // Fucking bools.
	void add(const char* field_name, const std::vector<vector2b>& values);
	void add(const char* field_name, const std::vector<vector3b>& values);
	void add(const char* field_name, const std::vector<vector4b>& values);

	// int
	void add(const char* field_name, const std::vector<int32_t>& values);
	void add(const char* field_name, const std::vector<vector2i>& values);
	void add(const char* field_name, const std::vector<vector3i>& values);
	void add(const char* field_name, const std::vector<vector4i>& values);

	// uint
	void add(const char* field_name, const std::vector<uint32_t>& values);
	void add(const char* field_name, const std::vector<vector2u>& values);
	void add(const char* field_name, const std::vector<vector3u>& values);
	void add(const char* field_name, const std::vector<vector4u>& values);

	// float
	void add(const char* field_name, const std::vector<float>& values);
	void add(const char* field_name, const std::vector<vector2>& values);
	void add(const char* field_name, const std::vector<vector3>& values);
	void add(const char* field_name, const std::vector<vector4>& values);
	void add(const char* field_name, const std::vector<matrix2>& values);
	void add(const char* field_name, const std::vector<matrix3>& values);
	void add(const char* field_name, const std::vector<matrix4>& values);

	// double
	void add(const char* field_name, const std::vector<double>& values);
	void add(const char* field_name, const std::vector<vector2d>& values);
	void add(const char* field_name, const std::vector<vector3d>& values);
	void add(const char* field_name, const std::vector<vector4d>& values);
	void add(const char* field_name, const std::vector<matrix2d>& values);
	void add(const char* field_name, const std::vector<matrix3d>& values);
	void add(const char* field_name, const std::vector<matrix4d>& values);

	// Creation of buffers from.
	virtual std::unique_ptr<ri_buffer> create_vertex_buffer(const char* name) = 0;
	virtual std::unique_ptr<ri_buffer> create_index_buffer(const char* name, const std::vector<uint16_t>& indices) = 0;
	virtual std::unique_ptr<ri_buffer> create_index_buffer(const char* name, const std::vector<uint32_t>& indices) = 0;

protected:
	virtual void add(const char* field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type) = 0;

};

}; // namespace ws
