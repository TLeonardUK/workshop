// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"
#include "workshop.core/drawing/color.h"

#include <span>

namespace ws {

// Defines what information is being queried.
enum class ri_query_type
{
    // Measures the elapsed time between the start and end of a query.
    time,
};

// ================================================================================================
//  Represents a query for information thats run in the gpu timeline.
// ================================================================================================
class ri_query
{
public:
    struct create_params
    {
        ri_query_type type = ri_query_type::time;
    };

public:
    virtual ~ri_query() {}
    
    virtual const char* get_debug_name() = 0;

    // Returns true if the results of this query are ready to return.
    virtual bool are_results_ready() = 0;

    // Gets the results of this query. Will block if are_results_ready
    // is false.
    virtual double get_results() = 0;
    
};

}; // namespace ws
