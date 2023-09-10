// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/string.h"
#include "workshop.core/math/math.h"
#include "workshop.core/utils/traits.h"

#include <functional>
#include <typeindex>

namespace ws {

class reflect_field;
class reflect_field_container_helper;
enum class reflect_field_container_type;

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
    using instance_create_t = std::function<void*()>;

    reflect_class(const char* name, std::type_index index, std::type_index parent, reflect_class_flags flags, const char* display_name, instance_create_t create_callback);
    virtual ~reflect_class();

    // Gets the name of this class.
    const char* get_name();

    // Gets the display name of this class.
    const char* get_display_name();

    // Returns true if the given flag is set.
    bool has_flag(reflect_class_flags flag);

    // Gets a field with the same name provided.
    reflect_field* find_field(const char* name, bool recursive = false);

    // Gets a list of all exposed fields in the class.
    std::vector<reflect_field*> get_fields(bool include_base_classes = false);

    // Gets the type index of the class being described.
    std::type_index get_type_index();

    // Returns true if thie given parent class is somwhere in the classes heirarchy.
    bool is_derived_from(reflect_class* parent);

    // Gets the parent class of this class.
    reflect_class* get_parent();

    // Creates an instance of this class.
    void* create_instance();

protected:
    void add_field(
        const char* name, 
        size_t offset, 
        size_t element_size, 
        std::type_index type, 
        std::type_index super_type, 
        const char* display_name, 
        const char* description, 
        reflect_field_container_type field_type, 
        std::unique_ptr<reflect_field_container_helper> container_helper);

    void add_constraint(const char* name, float min_value, float max_value);

private:
    std::vector<std::unique_ptr<reflect_field>> m_fields;
    std::string m_name;
    std::string m_display_name;
    std::type_index m_type_index;
    std::type_index m_parent_type_index;
    reflect_class_flags m_flags;
    instance_create_t m_create_function;


};

}; 