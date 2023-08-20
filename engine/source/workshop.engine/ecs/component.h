// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

// ================================================================================================
//  Base class for all components.
// 
//  Components should act as flat data structures, any logic should be performed in
//  the relevant systems.
// ================================================================================================
class component
{
public:
    virtual ~component() {};

};

}; // namespace ws
