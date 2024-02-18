// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset_importer.h"
#include "workshop.core/geometry/geometry.h"

#include <filesystem>

namespace ws {

class asset;
class ri_interface;
class renderer;
class asset_manager;

// ================================================================================================
//  Settings for importing a model.
// ================================================================================================
class model_importer_settings : public asset_importer_settings
{
public:
    virtual ~model_importer_settings() {};

    // Submeshes in a model are seperated into individual assets.
    bool seperate_submeshes = false;

    // All meshes imported will have their origin set to the bottom-center of their bounds.
    bool recalculate_origin = false;

    // Scales the vertices in a model.
    vector3 scale = vector3::one;

public:
    BEGIN_REFLECT(model_importer_settings, "Model Import Settings", asset_importer_settings, reflect_class_flags::none)
        REFLECT_FIELD(seperate_submeshes, "Seperate Submeshes", "Submeshes in the model file will be imported as seperate assets.")
        REFLECT_FIELD(recalculate_origin, "Recalculate Origin", "All meshes imported will have their origin set to the bottom-center of their bounds.")
        REFLECT_FIELD(scale, "Scale", "Scale applied to the overall mesh when imported.")
    END_REFLECT()

};

// ================================================================================================
//  Imports source folder files (fbx/obj/etc) and generates yaml asset files.
// ================================================================================================
class model_importer : public asset_importer
{
public:
    model_importer(ri_interface& instance, renderer& renderer, asset_manager& ass_manager);

    virtual std::vector<std::string> get_supported_extensions() override;
    virtual std::string get_file_type_description() override;
    virtual std::unique_ptr<asset_importer_settings> create_import_settings() override;
    virtual bool import(const char* source_path, const char* output_path, const asset_importer_settings & settings) override;

private:
    struct imported_texture
    {
        std::string name;
        std::string formatted_name;
        std::filesystem::path output_raw_path;
        std::filesystem::path output_yaml_path;
        std::string vfs_output_raw_path;
        std::string vfs_output_yaml_path;
        std::filesystem::path source_path;
        std::filesystem::path found_path;
        std::string usage = "color";
    };

    struct imported_material
    {
        std::string name;
        std::string formatted_name;
        std::filesystem::path output_path;
        std::string vfs_output_path;
        geometry_material material;
    };

    bool write_model_template(const char* path, const char* vfs_model_path, const std::vector<imported_material>& materials, const char* source_node_name, const model_importer_settings& import_settings);
    bool write_material_template(imported_material& material, const std::unordered_map<std::string, imported_texture>& textures);
    bool write_texture_template(imported_texture& texture);

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;
    asset_manager& m_asset_manager;

};

}; // namespace workshop
