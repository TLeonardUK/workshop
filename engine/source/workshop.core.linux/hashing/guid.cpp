// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/hashing/guid.h"

#include <uuid/uuid.h>

namespace ws {

guid guid::generate()
{
    static_assert(k_guid_size == sizeof(uuid_t));

    std::array<uint8_t, k_guid_size> bytes;
    uuid_generate(bytes.data());

    return guid(bytes);
}

}; // namespace workshop
