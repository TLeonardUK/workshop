// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/hashing/hash.h"
#include <string_view>

namespace ws {

// ================================================================================================
//  This is mostly based on:
//  https://stackoverflow.com/questions/35941045/can-i-obtain-c-type-names-in-a-constexpr-way
// 
//  It allows us to retrieve a template name and id in a constexpr form.
// ================================================================================================

template <typename T>
constexpr std::string_view type_name();

template <>
constexpr std::string_view type_name<void>()
{
    return "void";
}

namespace detail {

using type_name_prober = void;

template <typename T>
constexpr std::string_view get_wrapped_type_name() 
{
#ifdef __clang__
    return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    return __FUNCSIG__;
#else
#   error "Compiler unsupported."
#endif
}

constexpr std::size_t get_wrapped_type_name_prefix_length() 
{
    return get_wrapped_type_name<type_name_prober>().find(type_name<type_name_prober>());
}

constexpr std::size_t get_wrapped_type_name_suffix_length()
{
    return get_wrapped_type_name<type_name_prober>().length()
        - get_wrapped_type_name_prefix_length()
        - type_name<type_name_prober>().length();
}

}; // namespace detail

template <typename T>
constexpr std::string_view type_name() 
{
    constexpr std::string_view wrapped_name = detail::get_wrapped_type_name<T>();
    constexpr size_t prefix_length = detail::get_wrapped_type_name_prefix_length();
    constexpr size_t suffix_length = detail::get_wrapped_type_name_suffix_length();
    constexpr size_t type_name_length = wrapped_name.length() - prefix_length - suffix_length;
    return wrapped_name.substr(prefix_length, type_name_length);
}

template <typename T>
constexpr size_t type_id()
{
    constexpr std::string_view view = type_name<T>();
    return const_hash(view.data(), view.size());
}

// Defines operators for treating an enum as bitwise flags.
#define DEFINE_ENUM_FLAGS(t)                                                                    \
    inline t operator|(t a, t b)                                                                \
    {                                                                                           \
        using under_type = std::underlying_type<t>::type;                                       \
        return static_cast<t>(static_cast<under_type>(a) | static_cast<under_type>(b));         \
    }                                                                                           \
    inline t operator&(t a, t b)                                                                \
    {                                                                                           \
        using under_type = std::underlying_type<t>::type;                                       \
        return static_cast<t>(static_cast<under_type>(a) & static_cast<under_type>(b));         \
    }                                                                                           \
    inline t operator^(t a, t b)                                                                \
    {                                                                                           \
        using under_type = std::underlying_type<t>::type;                                       \
        return static_cast<t>(static_cast<under_type>(a) ^ static_cast<under_type>(b));         \
    }                                                                                           \
    inline t operator~(t a)                                                                     \
    {                                                                                           \
        using under_type = std::underlying_type<t>::type;                                       \
        return static_cast<t>(~static_cast<under_type>(a));                                     \
    }                                                                                           \
    inline t& operator!=(t& a, t b)                                                             \
    {                                                                                           \
        a = a | b;                                                                              \
        return a;                                                                               \
    }                                                                                           \
    inline t& operator&=(t& a, t b)                                                             \
    {                                                                                           \
        a = a & b;                                                                              \
        return a;                                                                               \
    }                                                                                           \
    inline t& operator^=(t& a, t b)                                                             \
    {                                                                                           \
        a = a ^ b;                                                                              \
        return a;                                                                               \
    }                                                                                           


}; // namespace workshop
