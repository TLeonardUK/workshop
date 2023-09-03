// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/texture/texture_importer.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/file.h"

#include <filesystem>
#include <unordered_map>

namespace ws {

texture_importer::texture_importer(ri_interface& instance, renderer& renderer, asset_manager& ass_manager)
    : m_ri_interface(instance)
    , m_renderer(renderer)
    , m_asset_manager(ass_manager)
{
}

std::vector<std::string> texture_importer::get_supported_extensions()
{
    return { 
        ".png", ".dds", ".tga", ".jpeg", ".jpg", ".bmp", ".psd", ".gif", ".hdr", ".pic", ".pnm"
    };
}

bool texture_importer::import(const char* in_source_path, const char* in_output_path)
{
    db_log(engine, "Importing Texture: %s", in_source_path);

    std::filesystem::path source_path = in_source_path;
    std::filesystem::path asset_name = string_replace(string_lower(source_path.filename().replace_extension().string().c_str()), " ", "_");
    std::filesystem::path asset_name_with_source_extension = asset_name;
    asset_name_with_source_extension = asset_name_with_source_extension.replace_extension(source_path.extension());
    std::filesystem::path asset_name_with_yaml_extension = asset_name;
    asset_name_with_yaml_extension = asset_name_with_yaml_extension.replace_extension(".yaml");

    std::filesystem::path output_raw_path = virtual_file_system::get().get_disk_location(in_output_path);
    std::filesystem::path output_raw_texture_path = output_raw_path / asset_name_with_source_extension;
    std::filesystem::path output_raw_texture_yaml_path = output_raw_path / asset_name_with_yaml_extension;

    std::filesystem::path output_vfs_path = in_output_path;
    std::filesystem::path output_vfs_texture_path = output_vfs_path / asset_name_with_source_extension;
    std::filesystem::path output_vfs_texture_yaml_path = output_vfs_path / asset_name_with_yaml_extension;
    std::string output_vfs_texture_path_normalized = virtual_file_system::normalize(output_vfs_texture_path.string().c_str());

    // Copy the source file over to our virtual file system.
    try
    {
        std::filesystem::copy_file(source_path, output_raw_texture_path, std::filesystem::copy_options::overwrite_existing);
    }
    catch (std::filesystem::filesystem_error& ex)
    {
        db_error(engine, "Failed to copy source file from '%s' to '%s': %s", source_path.string().c_str(), output_raw_texture_path.string().c_str(), ex.what());
        return false;
    }

    if (!write_texture_template(output_raw_texture_yaml_path.string().c_str(), output_vfs_texture_path_normalized.c_str()))
    {
        db_error(engine, "Failed to write out texture asset file: %s", output_raw_texture_yaml_path.string().c_str());
        return false;
    }

    return true;
}

bool texture_importer::write_texture_template(const char* raw_yaml_path, const char* vfs_texture_path)
{
    std::string output = "";
    std::string usage = "color";

    // Try and figure out what the texture is likely to be used for.
    std::string search = string_lower(vfs_texture_path);
    if (search.find("metallic") != std::string::npos)
    {
        usage = "metallic";
    }
    else if (search.find("roughness") != std::string::npos)
    {
        usage = "roughness";
    }
    else if (search.find("normal") != std::string::npos)
    {
        usage = "normal";
    }

    output += "# ================================================================================================\n";
    output += "#  workshop\n";
    output += "#  Copyright (C) 2023 Tim Leonard\n";
    output += "# ================================================================================================\n";
    output += "type: texture\n";
    output += "version: 1\n";
    output += "\n";
    output += "group: world\n";
    output += "usage: color\n";
    output += "\n";
    output += "faces:\n";
    output += "  - " + std::string(vfs_texture_path) + "\n";

    if (!write_all_text(raw_yaml_path, output))
    {
        return false;
    }

    return true;
}
}; // namespace ws

