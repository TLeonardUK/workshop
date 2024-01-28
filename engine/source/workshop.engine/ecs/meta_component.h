// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

namespace ws {

// Global flags describing the objects state. Mostly used for things
// like marking an object as selected in the editor without having
// to make user-level components to handle it.
enum class object_flags
{
    // Used for when flags have not been set yet - as an initial state.
    unset           = ~0,

    // No flags set.
    none            = 0,

    // Object is selected in the editor.
    selected        = 1,

    // Object is hidden in the scene tree in the editor. This is mostly used for
    // editor-internal objects like viewport cameras.
    hidden          = 2,

    // Object is never serialized when written to disk.
    transient       = 3,
};
DEFINE_ENUM_FLAGS(object_flags)

// ================================================================================================
//  This is a meta component thats added to all objects, it contains some backend information 
// 	that applicable to all object - debug names/etc.
// ================================================================================================
class meta_component : public component
{
public:

	// Debug name describing the object.
	std::string name = "unnamed";

    // Global flags describing the objects state. This flags are transient and are not serialized.
    object_flags flags = object_flags::none;

public:

    BEGIN_REFLECT(meta_component, "Meta", component, reflect_class_flags::abstract)
        REFLECT_FIELD(name, "Name", "Name shown in various parts of the editor to identify this object.")        
    END_REFLECT()

};

}; // namespace ws
