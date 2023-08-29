// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/string.h"
#include "workshop.core/math/math.h"
#include "workshop.core/utils/traits.h"

#include <typeindex>

namespace ws {

class reflect_field;

// Describes various aspects of a class.
enum class reflect_class_flags
{
    none            = 0,

    // The class is abstract and should not be constructed directly.
    abstract        = 1
};
DEFINE_ENUM_FLAGS(reflect_class_flags);

// ================================================================================================
//  Describes the contents of a given class.
// ================================================================================================
class reflect_class
{
public:
    reflect_class(const char* name, std::type_index index, std::type_index parent, reflect_class_flags flags, const char* display_name);
    virtual ~reflect_class();

    // Gets the name of this class.
    const char* get_name();

    // Gets the display name of this class.
    const char* get_display_name();

    // Returns true if the given flag is set.
    bool has_flag(reflect_class_flags flag);

    // Gets a field with the same name provided.
    reflect_field* find_field(const char* name);

    // Gets a list of all exposed fields in the class.
    std::vector<reflect_field*> get_fields();

    // Gets the type index of the class being described.
    std::type_index get_type_index();

    // Returns true if thie given parent class is somwhere in the classes heirarchy.
    bool is_derived_from(reflect_class* parent);

    // Gets the parent class of this class.
    reflect_class* get_parent();

protected:
    void add_field(const char* name, size_t offset, std::type_index type, const char* display_name, const char* description);
    void add_constraint(const char* name, float min_value, float max_value);

private:
    std::vector<std::unique_ptr<reflect_field>> m_fields;
    std::string m_name;
    std::string m_display_name;
    std::type_index m_type_index;
    std::type_index m_parent_type_index;
    reflect_class_flags m_flags;

};

}; 