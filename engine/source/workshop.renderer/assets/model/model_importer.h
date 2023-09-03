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
//  Imports source folder files (fbx/obj/etc) and generates yaml asset files.
// ================================================================================================
class model_importer : public asset_importer
{
public:
    model_importer(ri_interface& instance, renderer& renderer, asset_manager& ass_manager);

    virtual std::vector<std::string> get_supported_extensions() override;
    virtual bool import(const char* source_path, const char* output_path) override;

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

    bool write_model_template(const char* path, const char* vfs_model_path, const std::vector<imported_material>& materials);
    bool write_material_template(imported_material& material, const std::unordered_map<std::string, imported_texture>& textures);
    bool write_texture_template(imported_texture& texture);

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;
    asset_manager& m_asset_manager;

};

}; // namespace workshop
