// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/string.h"
#include "workshop.core/math/math.h"
#include "workshop.core/reflection/reflect_constraint.h"

#include <typeindex>

namespace ws {

// ================================================================================================
//  Base class for all constraints. Constraints define rules that limit how a reflection field is
//  modified in the editor - for example setting the allowable range.
// ================================================================================================
class reflect_constraint
{
public:
    virtual ~reflect_constraint() {};

};

// ================================================================================================
//  Constraint for limiting the editable range of a field.
// ================================================================================================
class reflect_constraint_range : public reflect_constraint
{
public:
    reflect_constraint_range(float min_value, float max_value);

    float get_min();
    float get_max();

private:
    float m_min_value;
    float m_max_value;

};

}; 