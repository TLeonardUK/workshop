// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/debug.h"

#include <string>
#include <unordered_map>

namespace ws {

// ================================================================================================
//  This class is the base class for protocol handlers that can be registered to the 
//  virtual file system.
// ================================================================================================
class stream
{
public:

    virtual ~stream() = default;

    // Flushes the stream and closes it, this is done implicitly on destruction.
    virtual void close() = 0;

    // Flushes any writes currently pending.
    virtual void flush() = 0;

    // Gets if this stream is opened for reading.
    virtual bool can_write() = 0;

    // Gets the position in the stream.
    virtual size_t position() = 0;

    // Gets the length of the entire stream.
    virtual size_t length() = 0;

    // Seeks to a specific location within the stream.
    virtual void seek(size_t position) = 0;

    // Writes the given bytes of data to the stream.
    // Returns the number of bytes written.
    virtual size_t write(const char* data, size_t size) = 0;

    // Reads the given bytes of data from the stream. 
    // Returns the number of bytes read.
    virtual size_t read(char* data, size_t size) = 0;

public:

    // Helper functions

    // How many bytes remaining in stream.
    size_t remaining();

    // If we have read to the end of the stream.
    bool at_end();

    // Reads the entire streams contents in as a string.
    std::string read_all_string();

    // Copies the contents of one stream to another in blocks.
    bool copy_to(stream& destination);
    
};

// ================================================================================================
//  General purpose stream serialization functions.
//  Add specializations for custom types.
// ================================================================================================

// Reads or writes the value from the stream depending on 
// if the stream is opened for writing or not.
template<typename type>
inline void stream_serialize(stream& out, type& value)
{
    //static_assert(false, "No specialization for this data type '%s'.", typeid());
    db_assert_message(false, "No serialize specialization for data type '%s'.", typeid(type).name());
}

template<typename type>
inline void stream_serialize_primitive(stream& out, type& value)
{
    if (out.can_write())
    {
        size_t bytes_wrote = out.write(reinterpret_cast<char*>(&value), sizeof(type));
        db_assert(bytes_wrote == sizeof(type));
    }
    else
    {
        size_t bytes_read = out.read(reinterpret_cast<char*>(&value), sizeof(type));
        db_assert(bytes_read == sizeof(type));
    }
}

template<> inline void stream_serialize(stream& out, bool& value)     { stream_serialize_primitive(out, value); }
template<> inline void stream_serialize(stream& out, uint8_t& value)  { stream_serialize_primitive(out, value); }
template<> inline void stream_serialize(stream& out, uint16_t& value) { stream_serialize_primitive(out, value); }
template<> inline void stream_serialize(stream& out, uint32_t& value) { stream_serialize_primitive(out, value); }
template<> inline void stream_serialize(stream& out, uint64_t& value) { stream_serialize_primitive(out, value); }
template<> inline void stream_serialize(stream& out, int8_t& value)   { stream_serialize_primitive(out, value); }
template<> inline void stream_serialize(stream& out, int16_t& value)  { stream_serialize_primitive(out, value); }
template<> inline void stream_serialize(stream& out, int32_t& value)  { stream_serialize_primitive(out, value); }
template<> inline void stream_serialize(stream& out, int64_t& value)  { stream_serialize_primitive(out, value); }
template<> inline void stream_serialize(stream& out, float& value)    { stream_serialize_primitive(out, value); }
template<> inline void stream_serialize(stream& out, double& value)   { stream_serialize_primitive(out, value); }

template<> inline void stream_serialize(stream& out, std::string& value)
{
    uint32_t len = static_cast<uint32_t>(value.size());
    stream_serialize_primitive(out, len);

    if (out.can_write())
    {
        size_t bytes_wrote = out.write(value.data(), len);
        db_assert(bytes_wrote == len);
    }
    else
    {
        value.resize(len);

        size_t bytes_read = out.read(value.data(), len);
        db_assert(bytes_read == len);
    }
}

template<typename type>
inline void stream_serialize_enum(stream& out, type& value)
{
    stream_serialize_primitive(out, value);
}

template<typename type>
inline void stream_serialize_list(stream& out, std::vector<type>& list, auto callback)
{
    uint32_t list_size = static_cast<uint32_t>(list.size());
    stream_serialize(out, list_size);

    if (!out.can_write())
    {
        list.resize(list_size);
    }

    for (size_t i = 0; i < list_size; i++)
    {
        callback(list[i]);
    }
}

template<typename type>
inline void stream_serialize_list(stream& out, std::vector<type>& list)
{
    uint32_t list_size = static_cast<uint32_t>(list.size());
    stream_serialize(out, list_size);

    if (!out.can_write())
    {
        list.resize(list_size);
    }

    for (size_t i = 0; i < list_size; i++)
    {
        stream_serialize(out, list[i]);
    }
}

template<typename type>
inline void stream_serialize_list_primitive(stream& out, std::vector<type>& list)
{
    uint32_t list_size = static_cast<uint32_t>(list.size());
    stream_serialize(out, list_size);

    size_t byte_size = list_size * sizeof(type);

    if (out.can_write())
    {
        size_t bytes_wrote = out.write(reinterpret_cast<char*>(list.data()), byte_size);
        db_assert(bytes_wrote == byte_size);
    }
    else
    {
        list.resize(list_size);

        size_t bytes_read = out.read(reinterpret_cast<char*>(list.data()), byte_size);
        db_assert(bytes_read == byte_size);
    }
}

template<> inline void stream_serialize_list(stream& out, std::vector<uint8_t>& value)  { stream_serialize_list_primitive(out, value); }
template<> inline void stream_serialize_list(stream& out, std::vector<uint16_t>& value) { stream_serialize_list_primitive(out, value); }
template<> inline void stream_serialize_list(stream& out, std::vector<uint32_t>& value) { stream_serialize_list_primitive(out, value); }
template<> inline void stream_serialize_list(stream& out, std::vector<uint64_t>& value) { stream_serialize_list_primitive(out, value); }
template<> inline void stream_serialize_list(stream& out, std::vector<int8_t>& value)   { stream_serialize_list_primitive(out, value); }
template<> inline void stream_serialize_list(stream& out, std::vector<int16_t>& value)  { stream_serialize_list_primitive(out, value); }
template<> inline void stream_serialize_list(stream& out, std::vector<int32_t>& value)  { stream_serialize_list_primitive(out, value); }
template<> inline void stream_serialize_list(stream& out, std::vector<int64_t>& value)  { stream_serialize_list_primitive(out, value); }
template<> inline void stream_serialize_list(stream& out, std::vector<float>& value)    { stream_serialize_list_primitive(out, value); }
template<> inline void stream_serialize_list(stream& out, std::vector<double>& value)   { stream_serialize_list_primitive(out, value); }

template<typename key_type, typename value_type>
inline void stream_serialize_map(stream& out, std::unordered_map<key_type, value_type>& list)
{
    uint32_t list_size = static_cast<uint32_t>(list.size());
    stream_serialize(out, list_size);

    if (out.can_write())
    {
        for (const std::pair<const key_type, value_type>& pair : list) 
        {
            key_type key = pair.first;
            value_type val = pair.second;

            stream_serialize(out, key);
            stream_serialize(out, val);
        }
    }
    else
    {
        for (size_t i = 0; i < list_size; i++)
        {
            key_type key;
            value_type val;

            stream_serialize(out, key);
            stream_serialize(out, val);

            list[key] = val;
        }
    }
}

}; // namespace workshop
