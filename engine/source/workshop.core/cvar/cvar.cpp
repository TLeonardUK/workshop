// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/cvar/cvar.h"
#include "workshop.core/cvar/cvar_manager.h"
#include "workshop.core/debug/debug.h"

namespace ws {

cvar_base::cvar_base(std::type_index value_type, cvar_flag flags, const char* name, const char* description)
    : m_value_type(value_type)
    , m_flags(flags)
    , m_name(name)
    , m_description(description)
{
    cvar_manager::get().register_cvar(this);
}

cvar_base::~cvar_base()
{
    cvar_manager::get().unregister_cvar(this);
}

void cvar_base::set_string(const char* value, cvar_source source)
{
    db_assert(m_value_type == typeid(std::string));
    set_variant(value, source);
}

void cvar_base::set_int(int value, cvar_source source)
{
    db_assert(m_value_type == typeid(int));
    set_variant(value, source);
}

void cvar_base::set_float(float value, cvar_source source)
{
    db_assert(m_value_type == typeid(float));
    set_variant(value, source);
}

void cvar_base::set_bool(bool value, cvar_source source)
{
    db_assert(m_value_type == typeid(bool));
    set_variant(value, source);
}

void cvar_base::set_variant(value_storage_t value, cvar_source source)
{
    // Already set by a high priority source.
    if (m_source > source)
    {
        return;
    }

    if (source == cvar_source::set_by_code_default ||
        source == cvar_source::set_by_config_default)
    {
        m_default_value = value;
        m_default_source = source;
    }

    if (m_value == value)
    {
        return;
    }

    value_storage_t old_value = m_value;

    m_value = value;
    m_source = source;

    on_changed.broadcast(old_value);

    if (has_flag(cvar_flag::evaluate_on_change))
    {
        // Don't re-evaluate if being set by default/save/etc.
        if (source != cvar_source::set_by_code_default &&
            source != cvar_source::set_by_config_default &&
            source != cvar_source::set_by_config &&
            source != cvar_source::set_by_save)
        {
            cvar_manager::get().evaluate();
        }
    }
}

void cvar_base::reset_to_default()
{
    set_variant(m_default_value, m_default_source);
}

const char* cvar_base::get_string()
{
    db_assert(m_value_type == typeid(std::string));
    return std::get<std::string>(m_value).c_str();
}

int cvar_base::get_int()
{
    db_assert(m_value_type == typeid(int));
    return std::get<int>(m_value);
}

bool cvar_base::get_bool()
{
    db_assert(m_value_type == typeid(bool));
    return std::get<bool>(m_value);
}

float cvar_base::get_float()
{
    db_assert(m_value_type == typeid(float));
    return std::get<float>(m_value);
}

bool cvar_base::has_flag(cvar_flag flag)
{
    return (m_flags & flag) == flag;
}

bool cvar_base::has_source(cvar_source source)
{
    return (m_source == source);
}

const char* cvar_base::get_name()
{
    return m_name.c_str();
}

const char* cvar_base::get_description()
{
    return m_description.c_str();
}

std::type_index cvar_base::get_value_type()
{
    return m_value_type;
}

}; // namespace ws
