// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/string.h"
#include "workshop.core/math/math.h"
#include "workshop.core/utils/traits.h"

#include <typeindex>

namespace ws {

// Describes various aspects of a enum.
enum class reflect_enum_flags
{
    none            = 0,

    // The enum is treated as a bitmask of flags.
    flags           = 1
};
DEFINE_ENUM_FLAGS(reflect_enum_flags);

// ================================================================================================
//  Describes the contents of a given enum.
// ================================================================================================
class reflect_enum
{
public:
    struct value
    {
        std::string name;
        std::string display_name;
        std::string description;
        int64_t value;
    };

public:
    reflect_enum(const char* name, std::type_index index, reflect_enum_flags flags, const char* display_name);
    virtual ~reflect_enum();

    // Gets the name of this class.
    const char* get_name();

    // Gets the display name of this class.
    const char* get_display_name();

    // Returns true if the given flag is set.
    bool has_flag(reflect_enum_flags flag);

    // Gets a value with the same name provided.
    const value* find_value(const char* name);

    // Gets a value with the same value provided.
    const value* find_value(int64_t value);

    // Gets a list of all values in the enum.
    std::vector<const value*> get_values();

    // Gets the type index of the class being described.
    std::type_index get_type_index();

protected:
    void add_value(
        const char* name, 
        int64_t value,
        const char* display_name,
        const char* description);

private:
    std::vector<std::unique_ptr<value>> m_values;
    std::string m_name;
    std::string m_display_name;
    std::type_index m_type_index;
    reflect_enum_flags m_flags;


};

}; 