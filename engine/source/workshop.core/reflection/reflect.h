// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/string.h"
#include "workshop.core/math/math.h"

#include "workshop.core/reflection/reflect_class.h"
#include "workshop.core/reflection/reflect_field.h"

#include <type_traits>
#include <typeindex>

namespace ws {

class reflect_class;

// Gets a reflect_class instance containing metadata about the given
// type given its type_index as a lookup.
// 
// If no metadata is registered for the class, returns nullptr.
reflect_class* get_reflect_class(std::type_index index);

// Wrapper function, gets the reflection class using the dynamic 
// typeid of the object pointer provided.
template<typename object_type>
reflect_class* get_reflect_class(object_type* type)
{
    return get_reflect_class(typeid(*type));
}

// Gets a reflect_class base on its name.
reflect_class* get_reflect_class(const char* name);

// Gets all reflect classes that derives from the given type.
std::vector<reflect_class*> get_reflect_derived_classes(std::type_index parent);

// Registers/Unregisters a reflection class.
void register_reflect_class(reflect_class* object);
void unregister_reflect_class(reflect_class* object);

// Class for helping manipulate vector reflection fields.
template <typename value_type>
class reflect_field_container_vector_helper 
    : public reflect_field_container_helper
{
    virtual void resize(void* container_ptr, size_t size) override
    {
        std::vector<value_type>* container = reinterpret_cast<std::vector<value_type>*>(container_ptr);
        container->resize(size);
    }

    virtual size_t size(void* container_ptr) override
    {
        std::vector<value_type>* container = reinterpret_cast<std::vector<value_type>*>(container_ptr);
        return container->size();
    }
    
    virtual void* get_data(void* container_ptr, size_t index) override
    {
        std::vector<value_type>* container = reinterpret_cast<std::vector<value_type>*>(container_ptr);
        return container->data() + index;
    }
};


// Macros for generating reflection data about a class. These should 
// be used inside the class like so:
//
// public:
//      BEGIN_REFLECT(transform_component, "Transform Component", component, reflect_class_flags::none)
//          REFLECT_FIELD(a, "A", "Does something something something")
//          REFLECT_CONSTRAINT_RANGE(a, 0, 10)
//      END_REFLECTIO()
//
// Eventually we will automate this when I'm less lazy and can be bothered to
// drag llvm and libclang into the project.

// Handy class for just passing into the parent argument if no parent needs to be set.
class reflect_no_parent { };

#define BEGIN_REFLECT(name, display_name, parent, flags)                                                                \
    class reflection : public reflect_class                                                                             \
    {                                                                                                                   \
    public:                                                                                                             \
        using class_t = name;                                                                                           \
        reflection()                                                                                                    \
            : reflect_class(#name, typeid(name), typeid(parent), flags, display_name, []() { return new name(); })      \
        {

#define REFLECT_FIELD(name, display_name, description)                                                                  \
            add_field(#name, offsetof(class_t, class_t::name), sizeof(decltype(class_t::name)), typeid(decltype(class_t::name)), typeid(void), display_name, description, reflect_field_container_type::scalar, nullptr); 

#define REFLECT_FIELD_REF(name, display_name, description)                                                              \
            add_field(#name, offsetof(class_t, class_t::name), sizeof(decltype(class_t::name)), typeid(decltype(class_t::name)), typeid(decltype(class_t::name)::super_type_t), display_name, description, reflect_field_container_type::scalar, nullptr); 

#define REFLECT_FIELD_LIST(name, display_name, description)                                                                  \
            add_field(#name, offsetof(class_t, class_t::name), sizeof(decltype(class_t::name)::value_type), typeid(decltype(class_t::name)::value_type), typeid(void), display_name, description, reflect_field_container_type::list, std::make_unique<reflect_field_container_vector_helper<decltype(class_t::name)::value_type>>()); 

#define REFLECT_FIELD_LIST_REF(name, display_name, description)                                                              \
            add_field(#name, offsetof(class_t, class_t::name), sizeof(decltype(class_t::name)::value_type), typeid(decltype(class_t::name)::value_type), typeid(decltype(class_t::name)::value_type::super_type_t), display_name, description, reflect_field_container_type::list, std::make_unique<reflect_field_container_vector_helper<decltype(class_t::name)::value_type>>()); 

#define REFLECT_CONSTRAINT_RANGE(name, min_val, max_val)                                                                \
            add_constraint(#name, static_cast<float>(min_val), static_cast<float>(max_val)); 

#define END_REFLECT()                                                                                                   \
        }                                                                                                               \
    };                                                                                                                  \
    inline static reflection self_class;

}; 