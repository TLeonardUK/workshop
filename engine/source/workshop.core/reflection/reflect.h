// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/string.h"
#include "workshop.core/math/math.h"

#include "workshop.core/reflection/reflect_class.h"
#include "workshop.core/reflection/reflect_enum.h"
#include "workshop.core/reflection/reflect_field.h"

#include <type_traits>
#include <typeindex>

namespace ws {

class reflect_class;
class reflect_enum;

// TODO:
//  Eventually we want to automate all of this by bringing libclang into the project
//  and preprocessing header files to extract all of this automatically.
//  Maybe one day the C++ committee will actually do something useful and get reflection working ...

// ---- Classes ----

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

// ---- Enums ----

// Gets a reflect_enum instance containing metadata about the given
// type given its type_index as a lookup.
// 
// If no metadata is registered for the class, returns nullptr.
reflect_enum* get_reflect_enum(std::type_index index);

// Wrapper function, gets the reflection class using the dynamic 
// typeid of the object pointer provided.
template<typename object_type>
reflect_enum* get_reflect_enum(object_type* type)
{
    return get_reflect_enum(typeid(*type));
}

// Gets a reflect_enum based on its name.
reflect_enum* get_reflect_enum(const char* name);

// Registers/Unregisters a reflection enum.
void register_reflect_enum(reflect_enum* object);
void unregister_reflect_enum(reflect_enum* object);

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
//      END_REFLECT()

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

// Simple reflection of a field.
#define REFLECT_FIELD(name, display_name, description)                                                                  \
            add_field(#name, offsetof(class_t, class_t::name), sizeof(decltype(class_t::name)), typeid(decltype(class_t::name)), typeid(void), typeid(void), display_name, description, reflect_field_container_type::scalar, nullptr); 

// Simple reflection of an enum field.
#define REFLECT_FIELD_ENUM(name, display_name, description)                                                                  \
            add_field(#name, offsetof(class_t, class_t::name), sizeof(decltype(class_t::name)), typeid(std::underlying_type_t<decltype(class_t::name)>), typeid(void), typeid(decltype(class_t::name)), display_name, description, reflect_field_container_type::enumeration, nullptr); 

// Reflect of a field that contains a reference, each asset_ptr, component_ref.
#define REFLECT_FIELD_REF(name, display_name, description)                                                              \
            add_field(#name, offsetof(class_t, class_t::name), sizeof(decltype(class_t::name)), typeid(decltype(class_t::name)), typeid(decltype(class_t::name)::super_type_t), typeid(void), display_name, description, reflect_field_container_type::scalar, nullptr); 

// Simple reflection of an std::vector that contains data you would normally reflect with REFLECT_FIELD.
#define REFLECT_FIELD_LIST(name, display_name, description)                                                                  \
            add_field(#name, offsetof(class_t, class_t::name), sizeof(decltype(class_t::name)::value_type), typeid(decltype(class_t::name)::value_type), typeid(void), typeid(void), display_name, description, reflect_field_container_type::list, std::make_unique<reflect_field_container_vector_helper<decltype(class_t::name)::value_type>>()); 

// Simple reflection of an std::vector that contains data you would normally reflect with REFLECT_FIELD_REF.
#define REFLECT_FIELD_LIST_REF(name, display_name, description)                                                              \
            add_field(#name, offsetof(class_t, class_t::name), sizeof(decltype(class_t::name)::value_type), typeid(decltype(class_t::name)::value_type), typeid(decltype(class_t::name)::value_type::super_type_t), typeid(void), display_name, description, reflect_field_container_type::list, std::make_unique<reflect_field_container_vector_helper<decltype(class_t::name)::value_type>>()); 

// Adds a constraint to a field, so that in editor its value cannot be moved out of the min and max range.
#define REFLECT_CONSTRAINT_RANGE(name, min_val, max_val)                                                                \
            add_constraint(#name, static_cast<float>(min_val), static_cast<float>(max_val)); 

// Adds a dependency to another class. What the dependency means is something thats handled at a higher level.
#define REFLECT_DEPENDENCY(class_type)                                                                                 \
            add_dependency(typeid(class_type)); 

#define END_REFLECT()                                                                                                   \
        }                                                                                                               \
    };                                                                                                                  \
    inline static reflection self_class;

// Macros for generating reflection data about an enum.
//
// public:
//  BEGIN_REFLECT_ENUM(render_draw_flags, "Draw Flags", reflect_enum_flags::flags)
//      REFLECT_ENUM(geometry, "Geometry", "Drawn by views that render the standard geometry passes.")
//      REFLECT_ENUM(editor, "Editor", "Drawn only by views that are rendering editor geometry.")
//  END_REFLECT_ENUM()

// Handy class for just passing into the parent argument if no parent needs to be set.
#define BEGIN_REFLECT_ENUM(name, display_name, flags)                                                                   \
    class reflect_enum_##name                                                                                           \
    {                                                                                                                   \
        class reflection : public reflect_enum                                                                          \
        {                                                                                                               \
        public:                                                                                                         \
            using enum_t = name;                                                                                        \
            reflection()                                                                                                \
                : reflect_enum(#name, typeid(name), flags, display_name)   \
            {

    // Simple reflection of a field.
    #define REFLECT_ENUM(name, display_name, description)                                                                  \
                add_value(#name, static_cast<int64_t>(enum_t::name), display_name, description);

    #define END_REFLECT_ENUM()                                                                                              \
            }                                                                                                               \
        };                                                                                                                  \
        inline static reflection self_class;                                                                                \
    };

}; 

