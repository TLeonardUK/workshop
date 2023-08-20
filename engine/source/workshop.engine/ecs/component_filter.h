// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

// ================================================================================================ 
//  A component filter allows you to retrieve a list of all objects and their associated components
//  that match the filter parameters.
// ================================================================================================
class component_filter
{
public:
    

};

/*
component_filter<camera_component, const fly_component> filter(m_system);
for (size_t i = 0; i < filter.size(); i++)
{
    object obj = filter.get_object(i);
    camera_component* camera = filter.get_component<camera_component>(i);
    const fly_component* fly = filter.get_component<const fly_component>(i);

    // do stuff
}
*/

}; // namespace ws
