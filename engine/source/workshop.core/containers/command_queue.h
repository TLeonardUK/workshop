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

namespace ws {

// ================================================================================================
//  
// ================================================================================================
class command
{
};

// ================================================================================================
//  
// ================================================================================================
class command_queue
{
public:
    command_queue();
    ~command_queue();

    void reset();

    template<typename command_type>
    void push();

    template<typename command_type>
    void pop();

    void push(command& cmd);
    command* pop();

};

}; // namespace workshop
