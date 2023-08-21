// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

class object_manager;

// Registers all the default game framework ECS systems to a given object manager.
void register_default_systems(object_manager& manager);

}; // namespace ws
