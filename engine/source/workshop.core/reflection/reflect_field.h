// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/string.h"
#include "workshop.core/math/math.h"

#include <typeindex>

namespace ws {

// ================================================================================================
//  Describes a field inside a class.
// ================================================================================================
class reflect_field
{
public:
    reflect_field(const char* name, size_t offset, std::type_index type, const char* display_name, const char* description);

    // Gets the name of this class.
    const char* get_name();

    // Gets the display name of this class.
    const char* get_display_name();

    // Gets the description of this class.
    const char* get_description();

    // Gets offset of field data in object instance.
    size_t get_offset();

    // Gets the type of this field.
    std::type_index get_type_index();

private:
    std::string m_name;
    std::string m_display_name;
    std::string m_description;

    size_t m_offset;
    std::type_index m_type;

};

}; 