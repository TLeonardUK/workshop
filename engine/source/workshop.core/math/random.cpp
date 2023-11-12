// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/math/random.h"
#include "workshop.core/math/math.h"

#include <random>

namespace {

static std::uniform_real_distribution<float> s_random_floats(0.0, 1.0);
static std::default_random_engine s_random_generator;

};

namespace ws::random {

quat random_quat()
{
    return quat::euler(vector3(
        s_random_floats(s_random_generator) * math::pi2,
        s_random_floats(s_random_generator) * math::pi2,
        s_random_floats(s_random_generator) * math::pi2
    ));
}

}; // namespace workshop
