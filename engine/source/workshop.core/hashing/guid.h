// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/containers/string.h"

#include <string>
#include <array>
#include <vector>
#include <functional>

namespace ws {

// ================================================================================================
//  Represents a globally unique identifier.
//  You can assume that all ids generated on all computers can be interchanged 
//  and should be unique.
// ================================================================================================
struct guid
{
public:

    // How many bytes make up the data of a guid.
    inline static const int k_guid_size = 16;

    // Sentinel value representing an uninitialized guid.
    static const guid k_empty;

public:
    
    guid();
    guid(const uint8_t data[k_guid_size]);
    guid(const std::array<uint8_t, k_guid_size>& data);
    guid(std::initializer_list<uint8_t> list);

    bool operator==(const guid& other) const;
    bool operator!=(const guid& other) const;

    const std::array<uint8_t, k_guid_size>& get_bytes() const;

    static guid generate();

private:
    std::array<uint8_t, k_guid_size> m_bytes;

};

inline const guid guid::k_empty = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// ================================================================================================
// Converts a hex-string (eg. 1BC3D3, etc) to a guid. Only supports pure hex-strings,
// strings with hyphens or other common formats are not supported.
// ================================================================================================
template <>
inline std::string to_string(const guid& input)
{
    const std::array<uint8_t, guid::k_guid_size>& input_bytes = input.get_bytes();

    std::vector<uint8_t> bytes(input_bytes.data(), input_bytes.data() + input_bytes.size());
    return to_hex_string(bytes);
}

// ================================================================================================
// Generates a hex-string representation (eg. 1BC3D3, etc) of this guid.
// ================================================================================================
template <>
inline result<guid> from_string(const std::string& input)
{
    result<std::vector<uint8_t>> parse_result = from_hex_string(input);
    if (parse_result)
    {
        std::vector<uint8_t> parse_bytes = parse_result.get_result();
        if (parse_bytes.size() != guid::k_guid_size)
        {
            return standard_errors::incorrect_length;
        }

        std::array<uint8_t, guid::k_guid_size> bytes;
        std::copy_n(parse_bytes.begin(), guid::k_guid_size, bytes.begin());

        return guid(bytes);
    }
    else
    {
        return parse_result.get_error();
    }
}

}; // namespace workshop

// ================================================================================================
// Specialization of std::hash for guid so we can use it in maps/etc.
// ================================================================================================
template<> 
struct std::hash<ws::guid> 
{
    std::size_t operator()(const ws::guid& k) const
    {
        std::size_t result = 5381;
        for (std::size_t i = 0; i < ws::guid::k_guid_size; i++)
        {
            result = k.get_bytes()[i] + 33 * result;
        }
        return result;
    }
};
