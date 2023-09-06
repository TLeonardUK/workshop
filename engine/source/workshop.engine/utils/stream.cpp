// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/utils/stream.h"
#include "workshop.core/utils/yaml.h"
#include "workshop.core/math/aabb.h"
#include "workshop.core/math/vector2.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/vector4.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/drawing/color.h"
#include "workshop.core/reflection/reflect_field.h"
#include "workshop.core/filesystem/stream.h"

#include "workshop.engine/ecs/component.h"
#include "workshop.assets/asset_manager.h"

namespace ws {

bool stream_serialize_reflect(stream& out, void* context, reflect_field* field)
{
    uint8_t* field_data = reinterpret_cast<uint8_t*>(context) + field->get_offset();

    // TODO: Argh, this is horrible, we need to figure out a better way to handle this.

    if (field->get_type_index() == typeid(int))
    {
        stream_serialize(out, *reinterpret_cast<int*>(field_data));
    }
    else if (field->get_type_index() == typeid(size_t))
    {
        stream_serialize(out, *reinterpret_cast<size_t*>(field_data));
    }
    else if (field->get_type_index() == typeid(float))
    {
        stream_serialize(out, *reinterpret_cast<float*>(field_data));
    }
    else if (field->get_type_index() == typeid(bool))
    {
        stream_serialize(out, *reinterpret_cast<bool*>(field_data));
    }
    else if (field->get_type_index() == typeid(aabb))
    {
        stream_serialize(out, *reinterpret_cast<aabb*>(field_data));
    }
    else if (field->get_type_index() == typeid(vector2))
    {
        stream_serialize(out, *reinterpret_cast<vector2*>(field_data));
    }
    else if (field->get_type_index() == typeid(vector3))
    {
        stream_serialize(out, *reinterpret_cast<vector3*>(field_data));
    }
    else if (field->get_type_index() == typeid(vector4))
    {
        stream_serialize(out, *reinterpret_cast<vector4*>(field_data));
    }
    else if (field->get_type_index() == typeid(quat))
    {
        stream_serialize(out, *reinterpret_cast<quat*>(field_data));
    }
    else if (field->get_type_index() == typeid(color))
    {
        stream_serialize(out, *reinterpret_cast<color*>(field_data));
    }
    else if (field->get_type_index() == typeid(std::string))
    {
        stream_serialize(out, *reinterpret_cast<std::string*>(field_data));
    }
    else if (field->get_super_type_index() == typeid(asset_ptr_base))
    {
        stream_serialize(out, *reinterpret_cast<asset_ptr_base*>(field_data));
    }
    else if (field->get_super_type_index() == typeid(component_ref_base))
    {
        stream_serialize(out, *reinterpret_cast<component_ref_base*>(field_data));
    }
    else
    {
        return false;
    }

    return true;
}

}; // namespace workshop
