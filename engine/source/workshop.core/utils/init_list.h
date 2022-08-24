// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/containers/string.h"

#include <string>
#include <array>
#include <vector>
#include <functional>

namespace ws {

// ================================================================================================
//  This list allows you to register multiple steps that consist of initializer/terminator pairs.
// 
//  When init is called all initializers are called sequentially, if one fails the terminators for 
//  each successfully initialized step are called in reverse order.
// 
//  term can be called at any point to terminate any currently initialized steps. term will
//  automatically be called when the object falls out of scope.
// ================================================================================================
struct init_list
{
public:

public:
    
    using init_function_t = std::function<result<void>()>;
    using term_function_t = std::function<result<void>()>;

    init_list();
    ~init_list();

    void add_step(const std::string& name, init_function_t init_func, term_function_t term_func);

    result<void> init();
    result<void> term();

private:
    struct step
    {
        std::string name;
        init_function_t init_func;
        term_function_t term_func;
        bool initialized;
    };

    std::vector<std::unique_ptr<step>> m_steps;

    bool m_init_running = false;
    size_t m_current_init_step = 0;
    size_t m_current_init_insert_step_count = 0;

};

}; // namespace workshop
