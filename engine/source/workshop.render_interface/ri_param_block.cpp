// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface/ri_param_block.h"

namespace ws {

bool ri_param_block::set(const char* field_name, const uint8_t& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(uint8_t)), sizeof(uint8_t), ri_data_type::t_bool);
}

bool ri_param_block::set(const char* field_name, const vector2b& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector2b)), sizeof(vector2b), ri_data_type::t_bool2);
}

bool ri_param_block::set(const char* field_name, const vector3b& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector3b)), sizeof(vector3b), ri_data_type::t_bool3);
}

bool ri_param_block::set(const char* field_name, const vector4b& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector4b)), sizeof(vector4b), ri_data_type::t_bool4);
}

bool ri_param_block::set(const char* field_name, const int32_t& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(int32_t)), sizeof(int32_t), ri_data_type::t_int);
}

bool ri_param_block::set(const char* field_name, const vector2i& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector2i)), sizeof(vector2i), ri_data_type::t_int2);
}

bool ri_param_block::set(const char* field_name, const vector3i& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector3i)), sizeof(vector3i), ri_data_type::t_int3);
}

bool ri_param_block::set(const char* field_name, const vector4i& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector4i)), sizeof(vector4i), ri_data_type::t_int4);
}

bool ri_param_block::set(const char* field_name, const uint32_t& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_uint);
}

bool ri_param_block::set(const char* field_name, const vector2u& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector2u)), sizeof(vector2u), ri_data_type::t_uint2);
}

bool ri_param_block::set(const char* field_name, const vector3u& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector3u)), sizeof(vector3u), ri_data_type::t_uint3);
}

bool ri_param_block::set(const char* field_name, const vector4u& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector4u)), sizeof(vector4u), ri_data_type::t_uint4);
}

bool ri_param_block::set(const char* field_name, const float& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(float)), sizeof(float), ri_data_type::t_float);
}

bool ri_param_block::set(const char* field_name, const vector2& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector2)), sizeof(vector2), ri_data_type::t_float2);
}

bool ri_param_block::set(const char* field_name, const vector3& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector3)), sizeof(vector3), ri_data_type::t_float3);
}

bool ri_param_block::set(const char* field_name, const vector4& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector4)), sizeof(vector4), ri_data_type::t_float4);
}

bool ri_param_block::set(const char* field_name, const matrix2& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix2)), sizeof(matrix2), ri_data_type::t_float2x2);
}

bool ri_param_block::set(const char* field_name, const matrix3& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix3)), sizeof(matrix3), ri_data_type::t_float3x3);
}

bool ri_param_block::set(const char* field_name, const matrix4& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix4)), sizeof(matrix4), ri_data_type::t_float4x4);
}

bool ri_param_block::set(const char* field_name, const double& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(double)), sizeof(double), ri_data_type::t_double);
}

bool ri_param_block::set(const char* field_name, const vector2d& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector2d)), sizeof(vector2d), ri_data_type::t_double2);
}

bool ri_param_block::set(const char* field_name, const vector3d& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector3d)), sizeof(vector3d), ri_data_type::t_double3);
}

bool ri_param_block::set(const char* field_name, const vector4d& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(vector4d)), sizeof(vector4d), ri_data_type::t_double4);
}

bool ri_param_block::set(const char* field_name, const matrix2d& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix2d)), sizeof(matrix2d), ri_data_type::t_double2x2);
}

bool ri_param_block::set(const char* field_name, const matrix3d& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix3d)), sizeof(matrix3d), ri_data_type::t_double4x4);
}

bool ri_param_block::set(const char* field_name, const matrix4d& values)
{
    return set(field_name, std::span((uint8_t*)&values, 1 * sizeof(matrix4d)), sizeof(matrix4d), ri_data_type::t_double4x4);
}

}; // namespace ws
