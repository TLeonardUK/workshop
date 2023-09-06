// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/filesystem/stream.h"

namespace ws {

class reflect_field;

// Performs serialization of a reflected field to a stream.
// Returns true if the type is supported and was correctly serialized.
bool stream_serialize_reflect(stream& out, void* context, reflect_field* field);

}; // namespace workshop
