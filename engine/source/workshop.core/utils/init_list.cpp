// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/utils/init_list.h"

namespace ws {

init_list::init_list()
{
}

init_list::~init_list()
{
    term();
}

void init_list::add_step(const std::string& name, init_function_t init_func, term_function_t term_func)
{
    std::unique_ptr<step> new_step = std::make_unique<step>();
    new_step->name = name;
    new_step->init_func = init_func;
    new_step->term_func = term_func;
    new_step->initialized = false;

    // If we are running initialization at the moment, slot this step in directly after out current step.
    if (m_init_running)
    {
        m_steps.insert(m_steps.begin() + m_current_init_step + m_current_init_insert_step_count + 1, std::move(new_step));
        m_current_init_insert_step_count++;
    }
    else
    {
        m_steps.push_back(std::move(new_step));
    }
}

result<void> init_list::init()
{
    m_init_running = true;

    // Don't use range iteration - steps may be added as part of initialization.
    for (size_t i = 0; i < m_steps.size(); i++)
    {
        step& step = *m_steps[i];

        m_current_init_step = i;
        m_current_init_insert_step_count = 0;

        if (!step.initialized)
        {
            db_log(core, "Initializing: %s", step.name.c_str());
            if (result<void> ret = step.init_func(); !ret)
            {
                db_log(core, "Failed to initialize step: %s", step.name.c_str());

                m_init_running = false;

                term();
                return ret;
            }
            else
            {
                step.initialized = true;
            }
        }
    }

    m_init_running = false;

    return true;
}

result<void> init_list::term()
{
    result<void> outcome = true;

    for (auto iter = m_steps.rbegin(); iter != m_steps.rend(); iter++)
    {
        step& step = *(*iter);

        if (step.initialized)
        {
            db_log(core, "Terminating: %s", step.name.c_str());
            if (result<void> ret = step.term_func(); !ret)
            {
                db_log(core, "Failed to terminate step: %s", step.name.c_str());

                outcome = ret;
            }

            step.initialized = false;
        }
    }

    return outcome;
}

}; // namespace workshop
