// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/log_handler.h"
#include "workshop.core/reflection/reflect.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/utils/event.h"
#include "workshop.core/drawing/color.h"

#include <functional>

namespace ws {

class reflect_class;

// ================================================================================================
//  Draws a list of editable properties for the given object.
// ================================================================================================
class property_list
{
public:

    property_list(void* obj, reflect_class* reflection_class);

    void draw();
    
    // Fired when a field in the context object has been modified.
    event<reflect_field*> on_modified;

protected:
    bool draw_edit(reflect_field* field, int& value, int min_value, int max_value);
    bool draw_edit(reflect_field* field, float& value, float min_value, float max_value);
    bool draw_edit(reflect_field* field, vector3& value, float min_value, float max_value);
    bool draw_edit(reflect_field* field, quat& value, float min_value, float max_value);
    bool draw_edit(reflect_field* field, bool& value);
    bool draw_edit(reflect_field* field, color& value);

private:
    
    void* m_context;
    reflect_class* m_class;

};

}; // namespace ws
