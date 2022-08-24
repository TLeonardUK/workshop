// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/log_handler.h"

namespace ws {

// ================================================================================================
//  Log handler that writes messages out to the terminal / console.
// ================================================================================================
struct log_handler_console
    : public log_handler
{
public:
    
    virtual void write(log_level level, const std::string& message) override;   

};

}; // namespace workshop
