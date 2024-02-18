// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

void ri_layout_factory::add(string_hash field_name, const std::vector<uint8_t>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(uint8_t)), sizeof(uint8_t), ri_data_type::t_bool);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector2b>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector2b)), sizeof(vector2b), ri_data_type::t_bool2);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector3b>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector3b)), sizeof(vector3b), ri_data_type::t_bool3);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector4b>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector4b)), sizeof(vector4b), ri_data_type::t_bool4);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<int32_t>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(int32_t)), sizeof(int32_t), ri_data_type::t_int);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector2i>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector2i)), sizeof(vector2i), ri_data_type::t_int2);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector3i>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector3i)), sizeof(vector3i), ri_data_type::t_int3);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector4i>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector4i)), sizeof(vector4i), ri_data_type::t_int4);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<uint32_t>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_uint);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector2u>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector2u)), sizeof(vector2u), ri_data_type::t_uint2);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector3u>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector3u)), sizeof(vector3u), ri_data_type::t_uint3);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector4u>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector4u)), sizeof(vector4u), ri_data_type::t_uint4);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<float>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(float)), sizeof(float), ri_data_type::t_float);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector2>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector2)), sizeof(vector2), ri_data_type::t_float2);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector3>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector3)), sizeof(vector3), ri_data_type::t_float3);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector4>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector4)), sizeof(vector4), ri_data_type::t_float4);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<matrix2>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix2)), sizeof(matrix2), ri_data_type::t_float2x2);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<matrix3>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix3)), sizeof(matrix3), ri_data_type::t_float3x3);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<matrix4>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix4)), sizeof(matrix4), ri_data_type::t_float4x4);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<double>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(double)), sizeof(double), ri_data_type::t_double);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector2d>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector2d)), sizeof(vector2d), ri_data_type::t_double2);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector3d>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector3d)), sizeof(vector3d), ri_data_type::t_double3);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<vector4d>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(vector4d)), sizeof(vector4d), ri_data_type::t_double4);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<matrix2d>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix2d)), sizeof(matrix2d), ri_data_type::t_double2x2);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<matrix3d>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix3d)), sizeof(matrix3d), ri_data_type::t_double4x4);
}

void ri_layout_factory::add(string_hash field_name, const std::vector<matrix4d>& values)
{
    add(field_name, std::span((uint8_t*)values.data(), values.size() * sizeof(matrix4d)), sizeof(matrix4d), ri_data_type::t_double4x4);
}

}; // namespace ws
