// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset.h"
#include "workshop.core/containers/string.h"

#include <array>
#include <unordered_map>

namespace ws {

class asset;
class asset_manager;
class world;
class engine;

// ================================================================================================
//  Scene assets contain the serialized state of a world, including all its objects and components
//  in a way that can easily be recreated.
// ================================================================================================
class scene : public asset
{
public:
    // Stores information on a component loaded from a scene file, before
    // its deserialized into an actual world.
    struct field_info
    {
        size_t field_name_index;
        size_t data_offset;
        size_t data_size;
    };

    struct component_info
    {
        size_t type_name_index;
        size_t field_offset;
        size_t field_count;
    };

    struct object_info
    {
        size_t handle;
        size_t component_offset;
        size_t component_count;
    };

public:
    // Loaded world, which can be made active in the engine via engine::set_default_world.
    world* world_instance = nullptr;

    // String table containing all component/field names, used when compiling.
    std::vector<std::string> string_table;

    // List of all objects in the scene, used when compiling.
    std::vector<object_info> objects;

    // List of all components in the scene, used when compiling.
    std::vector<component_info> components;

    // List of all fields in the scene, used when compiling.
    std::vector<field_info> fields;

    // Raw serialized field data.
    std::vector<uint8_t> data;

public:
    // Insert a string into the string_table and returns its index, or
    // returns the existing index if it already exists in the table.
    size_t intern_string(const char* string);

public:
    scene(asset_manager& ass_manager, engine* engine);
    virtual ~scene();

protected:
    virtual bool load_dependencies() override;

private:
    asset_manager& m_asset_manager;
    engine* m_engine;

};

}; // namespace workshop
