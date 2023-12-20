// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/event.h"
#include "workshop.core/utils/traits.h"

#include <variant>
#include <string>
#include <typeindex>

namespace ws {

// Bitmask of flags that define how a cvar is treated.
enum class cvar_flag
{
    // No special handling of cvar.
    none = 0,

    // Console variable is machine specific, data is not syncronized
    // between different devices.
    machine_specific = 1,

    // Cvar is serialized. If not set the cvar will always start with the default
    // value when the game starts.
    saved = 2,

    // When cvar is changed cvar files will be re-evaluated to pick up any settings
    // that are dependent on this one.
    evaluate_on_change = 3,
};
DEFINE_ENUM_FLAGS(cvar_flag);

// Defines where a cvars current value comes from. This also
// implictly defines a priority. If an attempt is made to set
// a cvar from a source when its already set by a higher source
// the set will be ignored.
enum class cvar_source
{
    // Nothing explicitly set.
    none = 0,

    // Default value defined in the C++ declaration.
    set_by_code_default = 1,

    // Default value defined in config file.
    set_by_config_default = 2,

    // Set to a value by configuration file.
    set_by_config = 3,

    // Set to a value by a save file.
    set_by_save = 4,

    // User has manually set this cvar setting.
    set_by_user = 5,

    // Some area of code has explicitly set the value.
    set_by_code = 6,
};

// Base class for all cvar's, don't use directly, use the 
// tempalted version below to set the value type.
class cvar_base
{
public:
    using value_storage_t = std::variant<std::string, int, float, bool>;
    
    cvar_base(std::type_index value_type, cvar_flag flags, const char* name, const char* description);
    ~cvar_base();

    void set_string(const char* value, cvar_source source = cvar_source::set_by_code);
    void set_int(int value, cvar_source source = cvar_source::set_by_code);
    void set_float(float value, cvar_source source = cvar_source::set_by_code);
    void set_bool(bool value, cvar_source source = cvar_source::set_by_code);

    void reset_to_default();

    const char* get_string();
    int get_int();
    bool get_bool();
    float get_float();

    bool has_flag(cvar_flag flag);
    bool has_source(cvar_source source);

    const char* get_name();
    const char* get_description();

    std::type_index get_value_type();

    // Called when the cvar's value is changed, parameter is the old value of the cvar.
    event<value_storage_t> on_changed;

private:
    void set_variant(value_storage_t value, cvar_source source = cvar_source::set_by_code);

private:    
    cvar_flag m_flags = cvar_flag::none;
    cvar_source m_source = cvar_source::none;
    std::string m_name;
    std::string m_description;
    std::type_index m_value_type;
    value_storage_t m_value;

    value_storage_t m_default_value;
    cvar_source m_default_source = cvar_source::none;
};

// cvar template that specifies the type of the value it holds. This is the 
// class that should be used throughout the engine.
template <typename value_type>
class cvar : public cvar_base
{
public:
    static_assert(
        std::is_same<value_type, std::string>::value ||
        std::is_same<value_type, float>::value ||
        std::is_same<value_type, int>::value ||
        std::is_same<value_type, bool>::value
    );

    cvar(cvar_flag flags, value_type default_value, const char* name, const char* description)
        : cvar_base(typeid(value_type), flags, name, description)
    {
        if constexpr (std::is_same<value_type, std::string>::value)
        {
            set_string(default_value, cvar_source::set_by_code_default);
        }
        else if constexpr (std::is_same<value_type, float>::value)
        {
            set_float(default_value, cvar_source::set_by_code_default);
        }
        else if constexpr (std::is_same<value_type, int>::value)
        {
            set_int(default_value, cvar_source::set_by_code_default);
        }
        else if constexpr (std::is_same<value_type, bool>::value)
        {
            set_bool(default_value, cvar_source::set_by_code_default);
        }
    }

    void set(value_type value)
    {
        if constexpr (std::is_same<value_type, std::string>::value)
        {
            set_string(value, cvar_source::set_by_code);
        }
        else if constexpr (std::is_same<value_type, float>::value)
        {
            set_float(value, cvar_source::set_by_code);
        }
        else if constexpr (std::is_same<value_type, int>::value)
        {
            set_int(value, cvar_source::set_by_code);
        }
        else if constexpr (std::is_same<value_type, bool>::value)
        {
            set_bool(value, cvar_source::set_by_code);
        }
    }

    value_type get()
    {
        if constexpr (std::is_same<value_type, std::string>::value)
        {
            return get_string();
        }
        else if constexpr (std::is_same<value_type, float>::value)
        {
            return get_float();
        }
        else if constexpr (std::is_same<value_type, int>::value)
        {
            return get_int();
        }
        else if constexpr (std::is_same<value_type, bool>::value)
        {
            return get_bool();
        }
    }
};

}; // namespace ws
