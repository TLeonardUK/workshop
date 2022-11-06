// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/containers/string.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <vector>

namespace ws {

std::string to_hex_string(std::vector<uint8_t>& input)
{
    std::stringstream ss;
    for (uint8_t Value : input)
    {
        ss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << (int)Value;
    }
    return ss.str();
}

result<std::vector<uint8_t>> from_hex_string(const std::string& input)
{
    if ((input.size() % 2) != 0)
    {
        return standard_errors::incorrect_length;
    }

    std::vector<uint8_t> result;
    result.resize(input.size() / 2);

    for (size_t i = 0; i < input.size(); i += 2)
    {
        char a = input[i];
        char b = input[i + 1];

        if (!is_char_hex(a) || !is_char_hex(b))
        {
            return standard_errors::incorrect_format;
        }

        uint8_t byte = (hex_char_to_int(a) << 4) | hex_char_to_int(b);
        result[i / 2] = byte;
    }

    return result;
}

std::string to_hex_display(std::vector<uint8_t>& input)
{
    static int column_width = 16;

    std::string result = "";

    for (size_t i = 0; i < input.size(); i += column_width)
    {
        std::string hex = "";
        std::string chars = "";

        for (size_t r = i; r < i + column_width && r < input.size(); r++)
        {
            uint8_t Byte = input[r];

            hex += string_format("%02X ", Byte);
            chars += is_char_renderable((char)Byte) ? (char)Byte : '.';
        }

        result += string_format("%-48s \xB3 %s\n", hex.c_str(), chars.c_str());
    }

    return result;
}

bool is_char_renderable(char c)
{
    return c >= 32 && c <= 126;
}

bool is_char_hex(char c)
{
    return (c >= '0' && c <= '9') || 
           (c >= 'a' && c <= 'f') || 
           (c >= 'A' && c <= 'F');
}

int hex_char_to_int(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    else if (c >= 'a' && c <= 'f')
    {
        return 10 + (c - 'a');
    }
    else if (c >= 'A' && c <= 'F')
    {
        return 10 + (c - 'A');
    }
    return 0;
}

std::string string_replace(const std::string& subject, const std::string& needle, const std::string& replacement, size_t start_offset)
{
    std::string result = subject;
    size_t position = start_offset;
    while (true)
    {
        position = result.find(needle, position);
        if (position == std::string::npos)
        {
            break;
        }
        result.replace(position, needle.length(), replacement);
        position += replacement.length();
    }
    return result;
}

std::string string_filter_out(const std::string& subject, const std::string& chars, char replacement_char, size_t start_offset)
{
    std::string result = subject;
    for (size_t i = start_offset; i < result.size(); i++)
    {
        if (chars.find(result[i]) != std::string::npos)
        {
            result[i] = replacement_char;
        }
    }
    return result;
}

bool string_ends_with(const std::string& subject, const std::string& needle)
{
    if (subject.size() >= needle.size())
    {
        size_t start_offset = subject.size() - needle.size();
        for (size_t i = start_offset; i < start_offset + needle.size(); i++)
        {
            if (subject[i] != needle[i - start_offset])
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool string_starts_with(const std::string& subject, const std::string& needle)
{
    if (subject.size() >= needle.size())
    {
        for (size_t i = 0; i < needle.size(); i++)
        {
            if (subject[i] != needle[i])
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool string_caseless_equals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
    {
        return false;
    }

    for (size_t i = 0; i < a.size(); i++)
    {
        if (std::toupper(a[i]) != std::toupper(b[i]))
        {
            return false;
        }
    }

    return true;
}

std::string string_trim(const std::string& subject, const std::string& chars_to_trim)
{
    int start_offset = 0;
    int end_offset = static_cast<int>(subject.size()) - 1;

    // Find first not of a trim char.
    for (; start_offset < subject.size(); start_offset++)
    {
        if (chars_to_trim.find_first_of(subject[start_offset]) == std::string::npos)
        {
            break;
        }
    }

    // Find last not of a trim char.
    for (; end_offset >= 0; end_offset--)
    {
        if (chars_to_trim.find_first_of(subject[end_offset]) == std::string::npos)
        {
            break;
        }
    }

    // Trimmed everything ...
    if (start_offset > end_offset)
    {
        return "";
    }

    return subject.substr(start_offset, (end_offset - start_offset) + 1);
}

std::string string_lower(const std::string& subject)
{
    std::string result;
    result.reserve(subject.size());

    for (size_t i = 0; i < subject.size(); i++)
    {
        result.push_back(std::tolower(subject[i]));
    }

    return result;
}

std::string string_upper(const std::string& subject)
{
    std::string result;
    result.reserve(subject.size());

    for (size_t i = 0; i < subject.size(); i++)
    {
        result.push_back(std::toupper(subject[i]));
    }

    return result;
}

uint32_t string_hash32(const std::string& subject)
{
    uint32_t hash = 0;

    for (size_t i = 0; i < subject.size(); i++)
    {
        hash = 37 * hash + subject[i];
    }

    return hash;
}

std::string string_join(const std::vector<std::string>& fragments, const std::string& glue)
{
    std::string result = "";

    for (const std::string& frag : fragments)
    {
        if (!result.empty())
        {
            result += glue;
        }
        result += frag;
    }

    return result;
}

std::vector<std::string> string_split(const std::string& value, const std::string& deliminator)
{
    std::vector<std::string> result;

    std::string remaining = value;

    while (remaining.size() > 0)
    {
        size_t pos = remaining.find(deliminator);
        if (pos != std::string::npos)
        {
            result.push_back(remaining.substr(0, pos));
            remaining = remaining.substr(pos + deliminator.size());
    
            // If deliminator was last character we have a "blank" string at the end.
            if (remaining.size() == 0)
            {
                result.push_back("");
            }
        }
        else
        {
            result.push_back(remaining);
            break;
        }
    }

    return result;
}

}; // namespace workshop
