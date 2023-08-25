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

};

}; // namespace ws
