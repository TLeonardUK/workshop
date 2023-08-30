// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/component.h"

namespace ws {

// ================================================================================================
//  This is a meta component thats added to all objects, it contains some backend information 
// 	that applicable to all object - debug names/etc.
// ================================================================================================
class meta_component : public component
{
public:

	// Debug name describing the object.
	std::string name = "unnamed";

public:

    BEGIN_REFLECT(meta_component, "Meta", component, reflect_class_flags::abstract)
        REFLECT_FIELD(name, "Name", "Name shown in various parts of the editor to identify this object.")        
    END_REFLECT()

};

}; // namespace ws
