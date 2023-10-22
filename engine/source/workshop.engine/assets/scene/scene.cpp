// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/scene/scene.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.core/reflection/reflect.h"
#include "workshop.core/reflection/reflect_class.h"
#include "workshop.core/reflection/reflect_field.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/ram_stream.h"

#include "workshop.core/utils/yaml.h"
#include "workshop.engine/utils/yaml.h"
#include "workshop.engine/utils/stream.h"
#include "workshop.engine/ecs/meta_component.h"

namespace ws {

scene::scene(asset_manager& ass_manager, engine* engine)
    : m_asset_manager(ass_manager)
    , m_engine(engine)
{
}

scene::~scene()
{
    if (world_instance)
    {
        m_engine->destroy_world(world_instance);
    }
}

size_t scene::intern_string(const char* string)
{
    auto iter = std::find_if(string_table.begin(), string_table.end(), [string](const std::string& value) {
        return value == string;
    });

    if (iter != string_table.end())
    {
        return std::distance(string_table.begin(), iter);
    }

    string_table.push_back(string);

    return string_table.size() - 1;
}

bool scene::load_dependencies()
{
    world_instance = m_engine->create_world(name.c_str());
    object_manager& obj_manager = world_instance->get_object_manager();

    ram_stream data_stream(data, false);

    // Instantiate all objects.
    for (object_info& obj_info : objects)
    {
        object handle = obj_manager.create_object((object)obj_info.handle);

        // Instantiate all components in object.
        for (size_t comp_index = obj_info.component_offset; comp_index < obj_info.component_offset + obj_info.component_count; comp_index++)
        {
            component_info& comp_info = components[comp_index];
            const std::string& type_name = string_table[comp_info.type_name_index];

            reflect_class* reflect_type = get_reflect_class(type_name.c_str());
            if (reflect_type == nullptr)
            {
                db_error(asset, "[%s] component node '%s' is of an unknown type.", name.c_str(), type_name.c_str());
                continue;
            }

            component* comp = obj_manager.add_component(handle, reflect_type->get_type_index());

            // Load all fields in component.
            for (size_t field_index = comp_info.field_offset; field_index < comp_info.field_offset + comp_info.field_count; field_index++)
            {
                field_info& info = fields[field_index];
                const std::string& field_name = string_table[info.field_name_index];

                reflect_field* reflect_field = reflect_type->find_field(field_name.c_str(), true);
                if (reflect_type == nullptr)
                {
                    db_error(asset, "[%s] field '%s::%s' is of an unknown type.", name.c_str(), type_name.c_str(), field_name.c_str());
                    continue;
                }

                data_stream.seek(info.data_offset);
                if (!stream_serialize_reflect(data_stream, comp, reflect_field))
                {
                    db_warning(engine, "[%s] Failed to serialize reflect field '%s::%s'.", name.c_str(), type_name.c_str(), field_name.c_str());
                    continue;
                }
            }
        }

        obj_manager.ensure_dependent_components_exist(handle);
    }

    // Mark all objects as modified.
    obj_manager.all_components_edited(component_modification_source::serialization);

    return true;
}

}; // namespace ws
