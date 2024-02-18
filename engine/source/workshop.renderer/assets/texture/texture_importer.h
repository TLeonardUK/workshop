// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset_importer.h"

namespace ws {

class asset;
class ri_interface;
class renderer;
class asset_manager;

// ================================================================================================
//  Settings for importing a texture.
// ================================================================================================
class texture_importer_settings : public asset_importer_settings
{
public:
    virtual ~texture_importer_settings() {};

public:
    BEGIN_REFLECT(texture_importer_settings, "Texture Import Settings", asset_importer_settings, reflect_class_flags::none)
    END_REFLECT()

};

// ================================================================================================
//  Imports source folder files (png/tga/etc) and generates yaml asset files.
// ================================================================================================
class texture_importer : public asset_importer
{
public:
    texture_importer(ri_interface& instance, renderer& renderer, asset_manager& ass_manager);

    virtual std::vector<std::string> get_supported_extensions() override;
    virtual std::string get_file_type_description() override;
    virtual std::unique_ptr<asset_importer_settings> create_import_settings() override;
    virtual bool import(const char* source_path, const char* output_path, const asset_importer_settings& settings) override;

private:
    bool write_texture_template(const char* raw_yaml_path, const char* vfs_texture_path);

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;
    asset_manager& m_asset_manager;

};

}; // namespace workshop
