// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/hashing/guid.h"

#include <random>

namespace ws {

guid guid::generate()
{
    static std::mt19937_64 engine(std::random_device{}());
    static std::uniform_int_distribution<int> distribution(0, 255);

    std::array<uint8_t, k_guid_size> bytes;
    for (uint8_t& byte : bytes)
    {
        byte = static_cast<uint8_t>(distribution(engine));
    }

    return guid(bytes);
}

}; // namespace ws
