// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/math.h"

#include <cmath>

namespace ws {

float compress_unit_vector(const vector3& vec)
{
    uint8_t x = math::float_to_byte(vec.x);
    uint8_t y = math::float_to_byte(vec.y);
    uint8_t z = math::float_to_byte(vec.z);
    uint32_t packed = (x << 16) | (y << 8) | z;
    return (float)packed;
}

vector3 decompress_unit_vector(float input)
{
    vector3 ret;

    // Unpack to 0-255 range.
    ret.x = floorf(input / 65536.0f);
    ret.y = floorf(fmod(input, 65536.0f) / 256.0f);
    ret.z = fmod(input, 256.0f);

    // Scale to -1 to 1 range.
    ret.x = (ret.x / 255.0f * 2.0f) - 1.0f;
    ret.y = (ret.y / 255.0f * 2.0f) - 1.0f;
    ret.z = (ret.z / 255.0f * 2.0f) - 1.0f;

    return ret.normalize();
}

}; // namespace workshop
