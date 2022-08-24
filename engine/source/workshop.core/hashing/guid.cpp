// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/hashing/guid.h"
#include "workshop.core/debug/debug.h"

#include <cassert>
#include <rpc.h>

namespace ws {

guid::guid()
    : guid(k_empty)
{
}

guid::guid(const uint8_t data[k_guid_size])
{
    memcpy(m_bytes.data(), data, k_guid_size);
}
    
guid::guid(std::initializer_list<uint8_t> list)
{
    db_assert(list.size() == k_guid_size);
    int i = 0;
    for (const uint8_t* iter = list.begin(); iter != list.end(); iter++, i++)
    {
        m_bytes[i] = *iter;
    }
}

guid::guid(const std::array<uint8_t, k_guid_size>& data)
    : m_bytes(data)
{
}

bool guid::operator==(const guid& other) const
{
    for (size_t i = 0; i < k_guid_size; i++)
    {
        if (m_bytes[i] != other.m_bytes[i])
        {
            return false;
        }
    }
    return true;
}

bool guid::operator!=(const guid& other) const
{
    return !operator==(other);
}

const std::array<uint8_t, guid::k_guid_size>& guid::get_bytes() const
{
    return m_bytes;
}

}; // namespace workshop
