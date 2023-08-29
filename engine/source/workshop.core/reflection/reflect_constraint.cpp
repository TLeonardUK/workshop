// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/reflection/reflect_constraint.h"

#include <algorithm>

namespace ws {

reflect_constraint_range::reflect_constraint_range(float min_value, float max_value)
    : m_min_value(min_value)
    , m_max_value(max_value)
{
}

float reflect_constraint_range::get_min()
{
    return m_min_value;
}

float reflect_constraint_range::get_max()
{
    return m_max_value;
}

}; // namespace workshop
