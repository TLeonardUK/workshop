// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/object.h"
#include "workshop.engine/ecs/object_manager.h"

#include "workshop.core/reflection/reflect.h"

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
template <typename component_type>
class component_ref
{
public:
    component_ref() = default;
    component_ref(object handle)
        : m_object(handle)
    {
    }

    object get_object() const
    {
        return m_object;
    }

    bool is_valid(object_manager* manager)
    {
        return get(manager) != nullptr;
    }

    component_type* get(object_manager* manager)
    {
        return manager->get_component<component_type>(m_object);
    }

    component_ref& operator=(object handle)
    {
        m_object = handle;
        return *this;
    }

    bool operator==(component_ref const& other) const
    {
        return m_object == other.m_object;
    }

    bool operator!=(component_ref const& other) const
    {
        return !operator==(other);
    }

    bool operator==(object const& other) const
    {
        return m_object == other;
    }

    bool operator!=(object const& other) const
    {
        return !operator==(other);
    }

private:
    object m_object = null_object;

};

}; // namespace ws
