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
//  Imports source folder files (png/tga/etc) and generates yaml asset files.
// ================================================================================================
class texture_importer : public asset_importer
{
public:
    texture_importer(ri_interface& instance, renderer& renderer, asset_manager& ass_manager);

    virtual std::vector<std::string> get_supported_extensions() override;
    virtual std::string get_file_type_description() override;
    virtual bool import(const char* source_path, const char* output_path) override;

private:
    bool write_texture_template(const char* raw_yaml_path, const char* vfs_texture_path);

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;
    asset_manager& m_asset_manager;

};

}; // namespace workshop
