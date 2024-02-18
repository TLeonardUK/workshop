// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"
#include "workshop.render_interface/ri_texture.h"

#include "workshop.core/math/vector2.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/vector4.h"
#include "workshop.core/math/matrix2.h"
#include "workshop.core/math/matrix3.h"
#include "workshop.core/math/matrix4.h"

#include "workshop.core/hashing/string_hash.h"

#include <span>

namespace ws {

class ri_texture;
class ri_sampler;
class ri_buffer;
class ri_param_block_archetype;
class ri_raytracing_tlas;

// ================================================================================================
//  Represents a block of parameters that can be passed into a shader as a constant buffer.
// ================================================================================================
class ri_param_block
{
public:
    virtual ~ri_param_block() {}

	// bool
	bool set(string_hash field_name, const uint8_t& values); // Fucking bools.
	bool set(string_hash field_name, const vector2b& values);
	bool set(string_hash field_name, const vector3b& values);
	bool set(string_hash field_name, const vector4b& values);

	// int
	bool set(string_hash field_name, const int32_t& values);
	bool set(string_hash field_name, const vector2i& values);
	bool set(string_hash field_name, const vector3i& values);
	bool set(string_hash field_name, const vector4i& values);

	// uint
	bool set(string_hash field_name, const uint32_t& values);
	bool set(string_hash field_name, const vector2u& values);
	bool set(string_hash field_name, const vector3u& values);
	bool set(string_hash field_name, const vector4u& values);

	// float
	bool set(string_hash field_name, const float& values);
	bool set(string_hash field_name, const vector2& values);
	bool set(string_hash field_name, const vector3& values);
	bool set(string_hash field_name, const vector4& values);
	bool set(string_hash field_name, const matrix2& values);
	bool set(string_hash field_name, const matrix3& values);
	bool set(string_hash field_name, const matrix4& values);

	// double
	bool set(string_hash field_name, const double& values);
	bool set(string_hash field_name, const vector2d& values);
	bool set(string_hash field_name, const vector3d& values);
	bool set(string_hash field_name, const vector4d& values);
	bool set(string_hash field_name, const matrix2d& values);
	bool set(string_hash field_name, const matrix3d& values);
	bool set(string_hash field_name, const matrix4d& values);

	// Resources
	virtual bool set(string_hash field_name, const ri_texture& resource) = 0;
	virtual bool set(string_hash field_name, const ri_texture_view& resource, bool writable = false) = 0;
	virtual bool set(string_hash field_name, const ri_sampler& resource) = 0;
	virtual bool set(string_hash field_name, const ri_buffer& resource, bool writable = false) = 0;
    virtual bool set(string_hash field_name, const ri_raytracing_tlas& resource) = 0;

	virtual bool clear_buffer(string_hash field_name) = 0;

	virtual ri_param_block_archetype* get_archetype() = 0;

    // Gets the index into the descriptor table of the buffer holding this param block
    // and the offset within that buffer of the param blocks data.
    virtual void get_table(size_t& index, size_t& offset) = 0;

private:
	virtual bool set(string_hash field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type) = 0;

};

}; // namespace ws
