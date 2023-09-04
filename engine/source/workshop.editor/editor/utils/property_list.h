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
#include "workshop.assets/asset_manager.h"

#include "workshop.renderer/assets/model/model.h"
#include "workshop.renderer/assets/texture/texture.h"
#include "workshop.renderer/assets/shader/shader.h"
#include "workshop.renderer/assets/material/material.h"

#include "workshop.engine/assets/asset_database.h"

#include <functional>

namespace ws {

class reflect_class;
class asset_manager;

// ================================================================================================
//  Draws a list of editable properties for the given object.
// ================================================================================================
class property_list
{
public:

    property_list(void* obj, reflect_class* reflection_class, asset_manager* ass_manager, asset_database& ass_database);

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
    bool draw_edit(reflect_field* field, std::string& value);
    bool draw_edit(reflect_field* field, asset_ptr<model>& value);

    void draw_preview(const char* path);

private:
    
    // When drawing an asset preview next to an editable field, what size should it be.
    static inline constexpr float k_preview_size = 64;

    // Padding around the inside of the asset preview and thumbnail.
    static inline constexpr float k_preview_padding = 2.0f;

    void* m_context;
    reflect_class* m_class;
    asset_manager* m_asset_manager;

    asset_database& m_asset_database;

};

}; // namespace ws
