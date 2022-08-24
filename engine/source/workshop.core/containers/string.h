// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/debug/debug.h"

#include <string>
#include <vector>
#include <memory>

namespace ws {

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

}; // namespace workshop
