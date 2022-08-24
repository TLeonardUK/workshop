// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/hashing/guid.h"

#include <rpc.h>

namespace ws {

guid guid::generate()
{
    UUID uuid;

    RPC_STATUS status = UuidCreate(&uuid);
    db_assert(status != RPC_S_UUID_NO_ADDRESS); // Huuum, not sure what we can do about that.
    
    std::array<uint8_t, k_guid_size> bytes;
    std::copy_n(reinterpret_cast<uint8_t*>(&uuid.Data1), k_guid_size, bytes.begin());

    return guid(bytes);
}

}; // namespace workshop
