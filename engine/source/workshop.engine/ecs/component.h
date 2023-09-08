// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/object.h"
#include "workshop.engine/ecs/object_manager.h"

#include "workshop.core/reflection/reflect.h"
#include "workshop.core/utils/yaml.h"
#include "workshop.core/filesystem/stream.h"

namespace ws {

// ================================================================================================
//  Base class for all components.
// 
//  Components should act as flat data structures, any logic should be performed in
//  the relevant systems.
// ================================================================================================
class component
{
public:
    virtual ~component() {};
    
public:

    BEGIN_REFLECT(component, "Component", reflect_no_parent, reflect_class_flags::none)
    END_REFLECT()

};

// ================================================================================================
//  Simple class that wraps a reference to an entities component.
// ================================================================================================
class component_ref_base
{
public:
    component_ref_base(std::type_index type) 
        : type_index(type)
    {
    }

    component_ref_base(object in_handle, std::type_index type)
        : handle(in_handle)
        , type_index(type)
    {
    }

    object get_object()
    {
        return handle;
    }

    std::type_index get_type_index()
    {
        return type_index;
    }

    object handle = null_object;
    std::type_index type_index;

};

template <typename component_type>
class component_ref : public component_ref_base
{
public:
    // This is used for reflection when using REFLECT_FIELD_REF
    using super_type_t = component_ref_base;

    component_ref()
        : component_ref_base(typeid(component_type))
    {
    }

    component_ref(object handle)
        : component_ref_base(handle, typeid(component_type))
    {
    }

    bool is_valid(object_manager* manager)
    {
        return get(manager) != nullptr;
    }

    component_type* get(object_manager* manager)
    {
        return manager->get_component<component_type>(handle);
    }

    component_ref& operator=(object in_handle)
    {
        handle = in_handle;
        return *this;
    }

    bool operator==(component_ref const& other) const
    {
        return handle == other.handle;
    }

    bool operator!=(component_ref const& other) const
    {
        return !operator==(other);
    }

    bool operator==(object const& other) const
    {
        return handle == other;
    }

    bool operator!=(object const& other) const
    {
        return !operator==(other);
    }

};

template<>
inline void stream_serialize(stream& out, component_ref_base& v)
{
    stream_serialize(out, v.handle);
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, component_ref_base& value)
{
    yaml_serialize(out, is_loading, value.handle);
}

}; // namespace ws
