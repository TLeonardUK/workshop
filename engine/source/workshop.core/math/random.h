// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/quat.h"
#include "workshop.core/math/matrix4.h"
#include "workshop.core/math/matrix3.h"

namespace ws::random {

// Generates a random float between 0 and 1.
float random_float();

// Generates a random quaternion.
quat random_quat();

// Generates a uniformly random rotation matrix.
matrix3 random_rotation_matrix();

}; // namespace ws::math