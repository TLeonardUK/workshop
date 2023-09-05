// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/reflection/reflect.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

namespace ws {

// Performs serialization of a reflected field to yaml.
// Returns true if the type is supported and was correctly serialized.
bool yaml_serialize_reflect(YAML::Node& out, bool is_loading, void* context, reflect_field* field);

}; // namespace workshop
