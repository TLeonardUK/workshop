// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/debug/debug.h"
#include "workshop.core/math/math.h"

#include <string>
#include <vector>
#include <memory>

namespace ws {

// ================================================================================================
//  Converts an array of bytes into a hex string. eg:
//      A1B2C3D46ACAE123
// ================================================================================================
std::string to_hex_string(std::vector<uint8_t>& input);

// ================================================================================================
//  Converts a hex string into an array of bytes. eg:
//      A1B2C3D46ACAE123
// ================================================================================================
result<std::vector<uint8_t>> from_hex_string(const std::string& input);

// ================================================================================================
//  Converts an array of bytes into a hex-editor style display.
//      AB123123123 | ..AC..A. 
//      AB123123123 | ..A.3.1. 
// ================================================================================================
std::string to_hex_display(std::vector<uint8_t>& input);

// ================================================================================================
//  Returns true if a character is renderable (in the ascii codepage)
// ================================================================================================
bool is_char_renderable(char c);

// ================================================================================================
//  Returns true if a character is valid in hexidecimal.
// ================================================================================================
bool is_char_hex(char c);

// ================================================================================================
//  Returns the integer value of a hex character.
// ================================================================================================
int hex_char_to_int(char c);

// ================================================================================================
//  Formats a string using the same syntax as printf.
// ================================================================================================
template<typename... Args>
inline std::string string_format(const char* format, Args... args)
{
    size_t size = static_cast<size_t>(snprintf(nullptr, 0, format, args ...)) + 1; // Extra space for '\0'

    std::unique_ptr<char[]> buffer(new char[size]);
    snprintf(buffer.get(), size, format, args ...);

    return std::string(buffer.get(), buffer.get() + size - 1); // We don't want the '\0' inside
}

// ================================================================================================
//  Converts a wide utf-16 string to utf-8.
// ================================================================================================
std::string narrow_string(const std::wstring& input);

// ================================================================================================
//  Converts a utf-8 string to a utf-16 string.
// ================================================================================================
std::wstring widen_string(const std::string& input);

// ================================================================================================
//  Replaces one string with another in a string.
// ================================================================================================
std::string string_replace(const std::string& subject, const std::string& needle, const std::string& replacement, size_t start_offset = 0);

// ================================================================================================
//  Replaces one st
// ================================================================================================
std::string string_filter_out(const std::string& subject, const std::string& chars, char replacement_char, size_t start_offset = 0);

// ================================================================================================
//  Determines if a given string ends with another string.
// ================================================================================================
bool string_ends_with(const std::string& subject, const std::string& needle);

// ================================================================================================
//  Determines if a given string starts with another string.
// ================================================================================================
bool string_starts_with(const std::string& subject, const std::string& needle);

// ================================================================================================
//  Determines if two strings are equal. This only handles simple case conversion (ascii).
// ================================================================================================
bool string_caseless_equals(const std::string& a, const std::string& b);

// ================================================================================================
//  Strips an array of characters from the start and end of a given string.
// ================================================================================================
std::string string_trim(const std::string& subject, const std::string& chars_to_trim);

// ================================================================================================
//  Converts a string to lowercase.
// ================================================================================================
std::string string_lower(const std::string& subject);

// ================================================================================================
//  Converts a string to uppercase.
// ================================================================================================
std::string string_upper(const std::string& subject);

// ================================================================================================
//  Generates a 32bit hash from a string. No guarantees are given about collisions
//  so be careful with your usage.
// ================================================================================================
uint32_t string_hash32(const std::string& hash);

// ================================================================================================
//  Joins a set of string fragments together with glue characters inbetween.
// ================================================================================================
std::string string_join(const std::vector<std::string>& fragments, const std::string& glue);

// ================================================================================================
//  Splits a string based on a deliminator.
// ================================================================================================
std::vector<std::string> string_split(const std::string& value, const std::string& deliminator);

// ================================================================================================
//  Base template function for converting arbitrary data type into strings.
//  Each type that is convertable should create a specialization.
// ================================================================================================
template <typename input_type>
inline std::string to_string(const input_type& input)
{
    // No implementation exists for this input type.
    db_assert(false);
}

// ================================================================================================
//  Base template function for converting strings into arbitrary data types.
//  Each type that is convertable should create a specialization.
// ================================================================================================
template <typename input_type>
inline result<input_type> from_string(const std::string& input)
{
    // No implementation exists for this input type.
    db_assert(false);
}

// ================================================================================================
//  Useful define for automatically creating to_string/from_string implementations for an enum.
//  The enum must have a COUNT entry at the end.
// ================================================================================================
#define DEFINE_ENUM_TO_STRING(type, string_array)                           \
    template <>                                                             \
    inline std::string to_string(const type& input)                         \
    {                                                                       \
        int input_int = static_cast<int>(input);                            \
        int platform_count = static_cast<int>(type::COUNT);                 \
                                                                            \
        if (math::in_range(input_int, 0, platform_count))                   \
        {                                                                   \
            return string_array[input_int];                                 \
        }                                                                   \
                                                                            \
        db_assert_message(false, "Invalid enum in conversion.");            \
        return "<unknown>";                                                 \
    }                                                                       \
    template <>                                                             \
    inline result<type> from_string(const std::string& input)               \
    {                                                                       \
        int platform_count = static_cast<int>(type::COUNT);                 \
                                                                            \
        for (size_t i = 0; i < platform_count; i++)                         \
        {                                                                   \
            if (string_array[i] == input)                                   \
            {                                                               \
                return static_cast<type>(i);                                \
            }                                                               \
        }                                                                   \
                                                                            \
        return standard_errors::not_found;                                  \
    }                                                                       

// ================================================================================================
//  Common to/from string overloads.
// ================================================================================================

// bool
template <>
inline std::string to_string(const bool& input)
{
    return input ? "true" : "false";
}

template <>
inline result<bool> from_string(const std::string& input)
{
    if (input == "true")
    {
        return true;
    }
    else if (input == "false")
    {
        return false;
    }
    return standard_errors::not_found;
}

// float
template <>
inline std::string to_string(const float& input)
{
    return std::to_string(input);
}

template <>
inline result<float> from_string(const std::string& input)
{
    return static_cast<float>(atof(input.c_str()));
}

// double
template <>
inline std::string to_string(const double& input)
{
    return std::to_string(input);
}

template <>
inline result<double> from_string(const std::string& input)
{
    return static_cast<float>(atof(input.c_str()));
}

// uint32_t
template <>
inline std::string to_string(const uint32_t& input)
{
    return std::to_string(input);
}

template <>
inline result<uint32_t> from_string(const std::string& input)
{
    try
    {
        return static_cast<uint32_t>(std::stoul(input.c_str()));
    }
    catch (std::invalid_argument)
    {
        return false;
    }
    catch (std::out_of_range)
    {
        return false;
    }
}

// int32_t
template <>
inline std::string to_string(const int32_t& input)
{
    return std::to_string(input);
}

template <>
inline result<int32_t> from_string(const std::string& input)
{
    try
    {
        return static_cast<int32_t>(std::stol(input.c_str()));
    }
    catch (std::invalid_argument)
    {
        return false;
    }
    catch (std::out_of_range)
    {
        return false;
    }
}

// uint64_t
template <>
inline std::string to_string(const uint64_t& input)
{
    return std::to_string(input);
}

template <>
inline result<uint64_t> from_string(const std::string& input)
{
    try
    {
        return static_cast<uint64_t>(std::stoull(input.c_str()));
    }
    catch (std::invalid_argument)
    {
        return false;
    }
    catch (std::out_of_range)
    {
        return false;
    }
}

// int64_t
template <>
inline std::string to_string(const int64_t& input)
{
    return std::to_string(input);
}

template <>
inline result<int64_t> from_string(const std::string& input)
{
    try
    {
        return static_cast<int64_t>(std::stoll(input.c_str()));
    }
    catch (std::invalid_argument)
    {
        return false;
    }
    catch (std::out_of_range)
    {
        return false;
    }
}

}; // namespace workshop
