// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface/ri_param_block.h"

namespace ws {

void ri_param_block::set(const char* field_name, const uint8_t& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(uint8_t)), sizeof(uint8_t), ri_data_type::t_bool);
}

void ri_param_block::set(const char* field_name, const vector2b& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector2b)), sizeof(vector2b), ri_data_type::t_bool2);
}

void ri_param_block::set(const char* field_name, const vector3b& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector3b)), sizeof(vector3b), ri_data_type::t_bool3);
}

void ri_param_block::set(const char* field_name, const vector4b& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector4b)), sizeof(vector4b), ri_data_type::t_bool4);
}

void ri_param_block::set(const char* field_name, const int32_t& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(int32_t)), sizeof(int32_t), ri_data_type::t_int);
}

void ri_param_block::set(const char* field_name, const vector2i& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector2i)), sizeof(vector2i), ri_data_type::t_int2);
}

void ri_param_block::set(const char* field_name, const vector3i& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector3i)), sizeof(vector3i), ri_data_type::t_int3);
}

void ri_param_block::set(const char* field_name, const vector4i& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector4i)), sizeof(vector4i), ri_data_type::t_int4);
}

void ri_param_block::set(const char* field_name, const uint32_t& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_uint);
}

void ri_param_block::set(const char* field_name, const vector2u& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector2u)), sizeof(vector2u), ri_data_type::t_uint2);
}

void ri_param_block::set(const char* field_name, const vector3u& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector3u)), sizeof(vector3u), ri_data_type::t_uint3);
}

void ri_param_block::set(const char* field_name, const vector4u& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector4u)), sizeof(vector4u), ri_data_type::t_uint4);
}

void ri_param_block::set(const char* field_name, const float& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(float)), sizeof(float), ri_data_type::t_float);
}

void ri_param_block::set(const char* field_name, const vector2& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector2)), sizeof(vector2), ri_data_type::t_float2);
}

void ri_param_block::set(const char* field_name, const vector3& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector3)), sizeof(vector3), ri_data_type::t_float3);
}

void ri_param_block::set(const char* field_name, const vector4& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector4)), sizeof(vector4), ri_data_type::t_float4);
}

void ri_param_block::set(const char* field_name, const matrix2& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix2)), sizeof(matrix2), ri_data_type::t_float2x2);
}

void ri_param_block::set(const char* field_name, const matrix3& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix3)), sizeof(matrix3), ri_data_type::t_float3x3);
}

void ri_param_block::set(const char* field_name, const matrix4& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix4)), sizeof(matrix4), ri_data_type::t_float4x4);
}

void ri_param_block::set(const char* field_name, const double& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(double)), sizeof(double), ri_data_type::t_double);
}

void ri_param_block::set(const char* field_name, const vector2d& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector2d)), sizeof(vector2d), ri_data_type::t_double2);
}

void ri_param_block::set(const char* field_name, const vector3d& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector3d)), sizeof(vector3d), ri_data_type::t_double3);
}

void ri_param_block::set(const char* field_name, const vector4d& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector4d)), sizeof(vector4d), ri_data_type::t_double4);
}

void ri_param_block::set(const char* field_name, const matrix2d& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix2d)), sizeof(matrix2d), ri_data_type::t_double2x2);
}

void ri_param_block::set(const char* field_name, const matrix3d& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix3d)), sizeof(matrix3d), ri_data_type::t_double4x4);
}

void ri_param_block::set(const char* field_name, const matrix4d& values)
{
    set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix4d)), sizeof(matrix4d), ri_data_type::t_double4x4);
}

}; // namespace ws
