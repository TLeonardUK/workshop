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

// Describes the type of value held in the field.
enum class reflect_field_container_type
{
    // Single value of the field type
    scalar = 0,

    // Behaves the same as the underlying scalar type, but contains the extra linking
    // to be able to query enum values.
    enumeration = 1,

    // List of values of the field type stored in an std::vector
    list = 2
};


// Derived versions of this class provide support for modifying a reflected container type.
class reflect_field_container_helper
{
public:
    virtual void   resize(void* container_ptr, size_t size) = 0;
    virtual size_t size(void* container_ptr) = 0;
    virtual void*  get_data(void* container_ptr, size_t index) = 0;
};

// ================================================================================================
//  Describes a field inside a class.
// ================================================================================================
class reflect_field
{
public:
    reflect_field(
        const char* name, 
        size_t offset, 
        size_t element_size, 
        std::type_index type, 
        std::type_index super_type, 
        std::type_index enum_type,
        const char* display_name, 
        const char* description, 
        reflect_field_container_type container_type, 
        std::unique_ptr<reflect_field_container_helper> helper);

    // Gets the name of this class.
    const char* get_name();

    // Gets the display name of this class.
    const char* get_display_name();

    // Gets the description of this class.
    const char* get_description();

    // Gets offset of field data in object instance.
    size_t get_offset();

    // Gets size of an element in the fields data. If field is a container, its the size of an individual element, otherwise its 
    // the size of the entire fields data.
    size_t get_element_size();

    // Gets the type of this field.
    std::type_index get_type_index();

    // Gets the super type of the type.
    std::type_index get_super_type_index();

    // Gets the enumeration type (for enums get_type_index is the underlying type, get_enum_type_index is the enum type).
    std::type_index get_enum_type_index();

    // Gets the type of container this field contains.
    reflect_field_container_type get_container_type();

    // Gets a class that provides helper functions for manipulating a container.
    reflect_field_container_helper* get_container_helper();

    // Adds a constraint to this field.
    void add_constraint(std::unique_ptr<reflect_constraint> constraint);

    // Gets a constraint of the given type.
    template <typename constraint_type>
    constraint_type* get_constraint()
    {
        for (auto& val : m_constraints)
        {
            constraint_type* type = dynamic_cast<constraint_type*>(val.get());
            if (type)
            {
                return type;
            }
        }
        return nullptr;
    }

private:
    std::string m_name;
    std::string m_display_name;
    std::string m_description;

    size_t m_offset;
    size_t m_element_size;
    std::type_index m_type;
    std::type_index m_super_type;
    std::type_index m_enum_type;

    reflect_field_container_type m_container_type;
    std::unique_ptr<reflect_field_container_helper> m_container_helper;

    std::vector<std::unique_ptr<reflect_constraint>> m_constraints;

};

}; 