// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/reflection/reflect_class.h"
#include "workshop.core/reflection/reflect_field.h"
#include "workshop.core/reflection/reflect_constraint.h"
#include "workshop.core/reflection/reflect.h"

#include <algorithm>

namespace ws {

reflect_class::reflect_class(const char* name, std::type_index index, std::type_index parent, reflect_class_flags flags, const char* display_name, instance_create_t create_function)
    : m_name(name)
    , m_type_index(index)
    , m_parent_type_index(parent)
    , m_display_name(display_name)
    , m_flags(flags)
    , m_create_function(create_function)
{
    register_reflect_class(this);
}

reflect_class::~reflect_class()
{
    unregister_reflect_class(this);
}

const char* reflect_class::get_name()
{
    return m_name.c_str();
}

const char* reflect_class::get_display_name()
{
    return m_display_name.c_str();
}

bool reflect_class::has_flag(reflect_class_flags flag)
{
    return (m_flags & flag) != reflect_class_flags::none;
}

std::type_index reflect_class::get_type_index()
{
    return m_type_index;
}

reflect_field* reflect_class::find_field(const char* name, bool recursive)
{
    auto iter = std::find_if(m_fields.begin(), m_fields.end(), [name](auto& field) {
        return strcmp(field->get_name(), name) == 0;
    });
    
    if (iter != m_fields.end())
    {
        return iter->get();
    }

    reflect_class* parent = get_parent();
    if (parent)
    {
        return parent->find_field(name, recursive);
    }
    else
    {
        return nullptr;
    }
}

std::vector<reflect_field*> reflect_class::get_fields(bool include_base_classes)
{
    std::vector<reflect_field*> ret;

    if (include_base_classes)
    {
        reflect_class* collect_class = this;
        while (collect_class)
        {
            std::vector<reflect_field*> class_fields = collect_class->get_fields(false);
            ret.insert(ret.end(), class_fields.begin(), class_fields.end());
            collect_class = collect_class->get_parent();
        }
    }
    else
    {
        for (auto& ptr : m_fields)
        {
            ret.push_back(ptr.get());
        }
    }

    return ret;
}

void reflect_class::add_field(const char* name, size_t offset, std::type_index type, std::type_index super_type,const char* display_name, const char* description)
{
    std::unique_ptr<reflect_field> field = std::make_unique<reflect_field>(name, offset, type, super_type, display_name, description);
    m_fields.push_back(std::move(field));
}

void reflect_class::add_constraint(const char* name, float min_value, float max_value)
{
    reflect_field* field = find_field(name);
    db_assert(field != nullptr);

    std::unique_ptr<reflect_constraint_range> constraint = std::make_unique<reflect_constraint_range>(min_value, max_value);
    field->add_constraint(std::move(constraint));
}

bool reflect_class::is_derived_from(reflect_class* parent)
{
    reflect_class* cls = get_reflect_class(m_parent_type_index);
    while (cls)
    {
        if (cls == parent)
        {
            return true;
        }
        cls = get_reflect_class(cls->m_parent_type_index);
    }
    return false;
}

reflect_class* reflect_class::get_parent()
{
    return get_reflect_class(m_parent_type_index);;
}

void* reflect_class::create_instance()
{
    return m_create_function();
}

}; // namespace workshop
