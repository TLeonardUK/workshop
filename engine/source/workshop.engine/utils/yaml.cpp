// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/utils/yaml.h"
#include "workshop.core/utils/yaml.h"
#include "workshop.core/math/aabb.h"
#include "workshop.core/math/vector2.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/vector4.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/drawing/color.h"

#include "workshop.engine/ecs/component.h"
#include "workshop.assets/asset_manager.h"

namespace ws {

bool yaml_serialize_reflect_internal(YAML::Node& node, bool is_loading, void* field_data, reflect_field* field, bool container_element)
{
    // TODO: Argh, this is horrible, we need to figure out a better way to handle this.

    if (!container_element && field->get_container_type() == reflect_field_container_type::list)
    {
        reflect_field_container_helper* helper = field->get_container_helper();
        size_t length = helper->size(field_data);

        if (is_loading)
        {
            length = node.size();
            helper->resize(field_data, length);
        }

        for (size_t i = 0; i < length; i++)
        {
            YAML::Node element_node;
            if (is_loading)
            {
                element_node = node[i];
            }

            if (!yaml_serialize_reflect_internal(element_node, is_loading, helper->get_data(field_data, i), field, true))
            {
                return false;
            }

            if (!is_loading)
            {
                node.push_back(element_node);
            }
        }
    }
    else if (field->get_type_index() == typeid(int))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<int*>(field_data));
    }
    else if (field->get_type_index() == typeid(size_t))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<size_t*>(field_data));
    }
    else if (field->get_type_index() == typeid(float))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<float*>(field_data));
    }
    else if (field->get_type_index() == typeid(bool))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<bool*>(field_data));
    }
    else if (field->get_type_index() == typeid(aabb))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<aabb*>(field_data));
    }
    else if (field->get_type_index() == typeid(vector2))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<vector2*>(field_data));
    }
    else if (field->get_type_index() == typeid(vector3))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<vector3*>(field_data));
    }
    else if (field->get_type_index() == typeid(vector4))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<vector4*>(field_data));
    }
    else if (field->get_type_index() == typeid(quat))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<quat*>(field_data));
    }
    else if (field->get_type_index() == typeid(color))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<color*>(field_data));
    }
    else if (field->get_type_index() == typeid(std::string))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<std::string*>(field_data));
    }
    else if (field->get_super_type_index() == typeid(asset_ptr_base))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<asset_ptr_base*>(field_data));
    }
    else if (field->get_super_type_index() == typeid(component_ref_base))
    {
        yaml_serialize(node, is_loading, *reinterpret_cast<component_ref_base*>(field_data));
    }
    else
    {
        return false;
    }

    return true;
}

bool yaml_serialize_reflect(YAML::Node& node, bool is_loading, void* context, reflect_field* field)
{
    uint8_t* field_data = reinterpret_cast<uint8_t*>(context) + field->get_offset();

    return yaml_serialize_reflect_internal(node, is_loading, field_data, field, false);
}

}; // namespace workshop
