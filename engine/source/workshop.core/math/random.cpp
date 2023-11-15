// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/math/random.h"
#include "workshop.core/math/math.h"

#include <random>

namespace {

static std::uniform_real_distribution<float> s_random_floats(0.0, 1.0);
static std::mt19937 s_random_generator;

};

namespace ws::random {

float random_float()
{
    return s_random_floats(s_random_generator);
}

quat random_quat()
{
    return random_rotation_matrix().to_quat();
}

matrix3 random_rotation_matrix()
{
    // Based on RTXGI code.

    // This approach is based on James Arvo's implementation from Graphics Gems 3 (pg 117-120).
    // Also available at: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.53.1357&rep=rep1&type=pdf

    // Setup a random rotation matrix using 3 uniform RVs
    float u1 = math::pi2 * random_float();
    float cos1 = cosf(u1);
    float sin1 = sinf(u1);

    float u2 = math::pi2 * random_float();
    float cos2 = cosf(u2);
    float sin2 = sinf(u2);

    float u3 = random_float();
    float sq3 = 2.f * sqrtf(u3 * (1.f - u3));

    float s2 = 2.f * u3 * sin2 * sin2 - 1.f;
    float c2 = 2.f * u3 * cos2 * cos2 - 1.f;
    float sc = 2.f * u3 * sin2 * cos2;

    // Create the random rotation matrix
    float _11 = cos1 * c2 - sin1 * sc;
    float _12 = sin1 * c2 + cos1 * sc;
    float _13 = sq3 * cos2;

    float _21 = cos1 * sc - sin1 * s2;
    float _22 = sin1 * sc + cos1 * s2;
    float _23 = sq3 * sin2;

    float _31 = cos1 * (sq3 * cos2) - sin1 * (sq3 * sin2);
    float _32 = sin1 * (sq3 * cos2) + cos1 * (sq3 * sin2);
    float _33 = 1.f - 2.f * u3;

    matrix3 transform;
    transform.set_column(0, { _11, _12, _13 });
    transform.set_column(1, { _21, _22, _23 });
    transform.set_column(2, { _31, _32, _33 });

    return transform;
}

}; // namespace workshop
