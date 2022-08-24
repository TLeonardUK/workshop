// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/log.h"

#include <memory>
#include <mutex>

namespace ws {

// ================================================================================================
//  Simple singleton base class.
//  Will assert if more than one instance of the derived class is instantiated.
// ================================================================================================
template <typename super_class>
class singleton
{
public:

    singleton()
    {
        db_assert(g_instance == nullptr);
        g_instance = static_cast<super_class*>(this);
    }

    virtual ~singleton()
    {
        db_assert(g_instance == this);
        g_instance = nullptr;
    }

    static super_class& get()
    {
        db_assert(g_instance != nullptr);
        return *g_instance;
    }

protected:

   inline static super_class* g_instance = nullptr;

};

// ================================================================================================
//  Same as singleton except the class will be auto-instantiated when 
//  the first attempt is made to access it.
// 
//  Be very careful accessing dependencies when the singleton is being destroyed 
//  as with this you have an undefined destruction order.
// ================================================================================================
template <typename super_class>
class auto_create_singleton
{
public:

    auto_create_singleton()
    {
        db_assert(g_instance == nullptr);
    }

    virtual ~auto_create_singleton()
    {
        db_assert(g_instance.get() == this);
        g_instance = nullptr;
    }

    static super_class& get()
    {
        if (g_instance == nullptr)
        {
            std::scoped_lock lock(g_create_mutex);
            if (g_instance == nullptr)
            {
                g_instance = std::make_shared<super_class>();
            }
        }

        db_assert(g_instance != nullptr);
        return *g_instance;
    }

protected:

    inline static std::shared_ptr<super_class> g_instance = nullptr;
    inline static std::mutex g_create_mutex;

};

}; // namespace workshop
