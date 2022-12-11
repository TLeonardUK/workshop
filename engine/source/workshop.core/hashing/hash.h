// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <memory>

namespace ws {

// Super cheap and simple constexpr string hash.
constexpr size_t const_hash(const char* data, size_t length) 
{
    size_t result = 5381;
    for (size_t i = 0; i < length; i++)
    {
        result = (result * 33) + data[i];
    }
    return result;
}

// Hash generator for std::pair types. Allows us to use pairs as keys in std containers.
struct std_pair_hash
{
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2>& p) const
    {
        auto h1_hasher = std::hash<T1> { };
        auto h2_hasher = std::hash<T2> { };
        auto h1 = h1_hasher(p.first);
        auto h2 = h2_hasher(p.second);
        return h1 ^ (h2 << 1);
    }
};

// Combines to std::hash values together.
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}; // namespace workshop
