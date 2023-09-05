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
    /*struct component_field
    {
        std::string name_index;

        size_t data_offset;
        size_t data_size;
    };

    struct component_info
    {
        std::string name_index;
        std::vector<component_field> fields;
    };

    struct object*/

public:
    // Loaded world, which can be made active in the engine via engine::set_default_world.
    world* world;

    // String table containing all component/field names, used when compiling.
    //std::vector<std::string> string_table;

    // List of all components in the scene, used when compiling.
    //std::vector<component_info> components;

public:
    scene(asset_manager& ass_manager, engine* engine);
    virtual ~scene();

protected:
    virtual bool post_load() override;

private:
    asset_manager& m_asset_manager;
    engine* m_engine;

};

}; // namespace workshop
