// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/model/model_importer.h"
#include "workshop.renderer/assets/texture/texture_importer.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/filesystem/file.h"

#include <filesystem>
#include <unordered_map>

#pragma optimize("", off)

namespace ws {
namespace {

std::string sanitize_filename(const char* path)
{
    std::string result = path;

    result = string_lower(result);
    result = string_filter_alphanum(result, '_');

    // Limit length of filenames to avoid ridiculously long filenames. 
    // Things like assimp result in node names that are recurisively concatanated and get very long.
    if (result.size() > 64)
    {
        result = result.substr(result.size() - 64);
    }

    return result;
}

};

model_importer::model_importer(ri_interface& instance, renderer& renderer, asset_manager& ass_manager)
    : m_ri_interface(instance)
    , m_renderer(renderer)
    , m_asset_manager(ass_manager)
{
}

std::vector<std::string> model_importer::get_supported_extensions()
{
    return { 
        ".3d", ".3ds", ".3mf", ".ac", ".amf", ".ase", ".assbin", ".assjson", ".assxml", ".b3d",
        ".blend", ".bvh", ".cob", ".collada", ".csm", ".dxf", ".fbx", ".gltf", ".hmp", ".ifc",
        ".iqm", ".irr", ".irrmesh", ".lwo", ".lws", ".m3d", ".md2", ".md3", ".md5", ".mdc", ".mdl",
        ".mmd", ".ms3d", ".ndo", ".nff", ".obj", ".off", ".ogre", ".opengex", ".pbrt", ".ply", ".q3bsp",
        ".q3d", ".raw", ".sib", ".smd", ".step", ".stl", ".terragen", ".x", ".x3d", ".xgl"    
    };
}

std::string model_importer::get_file_type_description()
{
    return "Model Files";
}

std::unique_ptr<asset_importer_settings> model_importer::create_import_settings()
{
    return std::make_unique<model_importer_settings>();
}

bool model_importer::import(const char* in_source_path, const char* in_output_path, const asset_importer_settings& settings)
{
    db_log(engine, "Importing model: %s", in_source_path);

    const model_importer_settings& import_settings = static_cast<const model_importer_settings&>(settings);

    std::filesystem::path source_path = in_source_path;
    std::filesystem::path asset_name = sanitize_filename(source_path.filename().replace_extension().string().c_str());
    std::filesystem::path asset_name_with_source_extension = asset_name;
    asset_name_with_source_extension = asset_name_with_source_extension.replace_extension(source_path.extension());
    std::filesystem::path asset_name_with_yaml_extension = asset_name;
    asset_name_with_yaml_extension = asset_name_with_yaml_extension.replace_extension(".yaml");
    std::filesystem::path source_directory = source_path.parent_path();

    std::filesystem::path output_path = virtual_file_system::get().get_disk_location(in_output_path);
    std::filesystem::path output_directory_path = output_path / asset_name;
    std::filesystem::path output_directory_texture_path = output_directory_path / "textures";
    std::filesystem::path output_directory_material_path = output_directory_path / "materials";

    std::filesystem::path vfs_output_directory_texture_path = std::filesystem::path(in_output_path) / asset_name / "textures";
    std::filesystem::path vfs_output_directory_material_path = std::filesystem::path(in_output_path) / asset_name / "materials";

    std::filesystem::path output_model_path = output_directory_path / asset_name_with_source_extension;
    std::filesystem::path output_model_yaml_path = output_directory_path / asset_name_with_yaml_extension;

    std::filesystem::path vfs_output_model_path = std::filesystem::path(in_output_path) / asset_name / asset_name_with_source_extension;
    std::string vfs_output_model_path_normalized = virtual_file_system::normalize(vfs_output_model_path.string().c_str());

    // Make sure the directory for the assets is created.
    try
    {
        if (!std::filesystem::exists(output_directory_path))
        {
            std::filesystem::create_directories(output_directory_path);
        }
        if (!std::filesystem::exists(output_directory_texture_path))
        {
            std::filesystem::create_directories(output_directory_texture_path);
        }
        if (!std::filesystem::exists(output_directory_material_path))
        {
            std::filesystem::create_directories(output_directory_material_path);
        }
    }
    catch (std::filesystem::filesystem_error& ex)
    {
        db_error(engine, "Failed to create asset directories inside '%s': %s", output_directory_path.string().c_str(), ex.what());
        return false;
    }

    // Copy the source file over to our virtual file system.
    try
    {
        std::filesystem::copy_file(source_path, output_model_path, std::filesystem::copy_options::overwrite_existing);
    }
    catch (std::filesystem::filesystem_error& ex)
    {
        db_error(engine, "Failed to copy source file from '%s' to '%s': %s", source_path.string().c_str(), output_model_path.string().c_str(), ex.what());
        return false;
    }

    // Write out the model template.
    
    // Load the fbx so we know what materials/etc we will need.
    geometry_load_settings geo_settings;
    geo_settings.merge_submeshes = !import_settings.seperate_submeshes;
    geo_settings.recalculate_origin = import_settings.recalculate_origin;
    geo_settings.scale = import_settings.scale;
    geo_settings.high_quality = true;

    std::unique_ptr<geometry> geometry = geometry::load(vfs_output_model_path_normalized.c_str(), geo_settings);

    // Figure out the materials we will need.
    std::vector<imported_material> materials;
    std::unordered_map<std::string, imported_texture> textures;

    // Build list of files we can use for file matching.
    std::vector<std::filesystem::path> file_matching_pool; 
    try
    {
        for (const std::filesystem::directory_entry& dir_entry : std::filesystem::recursive_directory_iterator(source_directory))
        {
            if (dir_entry.is_directory())
            {
                continue;
            }

            std::filesystem::path potential_path = dir_entry.path();
            file_matching_pool.push_back(potential_path);
        }
    }
    catch (std::filesystem::filesystem_error& ex)
    {
        db_error(engine, "Failed when building file searching path pool: %s", ex.what());
        return false;
    }
    
    // TODO: Move these lambdas into actual functions, this is messy.
    // TODO: Also make these functions less gross, these are brute forced the to extreme.

    texture_importer* tex_importer = m_asset_manager.get_importer<texture_importer>();
    std::vector<std::string> texture_extensions = tex_importer->get_supported_extensions();

    auto find_texture = [&file_matching_pool, &texture_extensions](const std::vector<const char*>& tags) -> std::string
    {
        for (const std::filesystem::path& potential_path : file_matching_pool)
        {
            std::string extension = potential_path.extension().string();
            if (std::find(texture_extensions.begin(), texture_extensions.end(), extension) == texture_extensions.end())
            {
                continue;
            }

            std::string string_path = string_lower(potential_path.string().c_str());
            for (const char* tag : tags)
            {
                if (strstr(string_path.c_str(), tag) != 0)
                {
                    return potential_path.string();
                }
            }
        }

        return "";
    };
    
    auto add_texture = [&file_matching_pool, &textures, &output_directory_texture_path, &vfs_output_directory_texture_path, &source_directory](geometry_texture& texture, const char* usage)
    {
        if (texture.path.empty())
        {
            return true;
        }

        auto iter = textures.find(texture.path);
        if (iter != textures.end())
        {
            return true;
        }

        std::string extension = std::filesystem::path(texture.path).extension().string();

        imported_texture& tex = textures[texture.path];
        tex.name = std::filesystem::path(texture.path).filename().replace_extension().string().c_str();
        tex.formatted_name = string_replace(string_lower(tex.name), " ", "_");
        tex.source_path = texture.path;
        tex.output_yaml_path = output_directory_texture_path / (tex.formatted_name + ".yaml");
        tex.output_raw_path = output_directory_texture_path / (tex.formatted_name + extension);
        tex.vfs_output_yaml_path = virtual_file_system::normalize((vfs_output_directory_texture_path / (tex.formatted_name + ".yaml")).string().c_str());
        tex.vfs_output_raw_path = virtual_file_system::normalize((vfs_output_directory_texture_path / (tex.formatted_name + extension)).string().c_str());
        tex.usage = usage;

        std::filesystem::path filename = std::filesystem::path(texture.path).filename();
        std::string filename_normalized = string_lower(filename.string());

        db_log(engine, "Importing texture: %s", tex.output_yaml_path.string().c_str());
        
        // Search in all folders under the folder the model was in for the texture.
        for (const std::filesystem::path& potential_path : file_matching_pool)
        {
            std::string potential_normalized = string_lower(potential_path.filename().string());

            if (potential_normalized == filename_normalized)
            {
                tex.found_path = potential_path;
                break;
            }
        }

        if (tex.found_path.empty())
        {
            db_error(engine, "Failed to find source texture, check it's named correctly: %s", tex.output_raw_path.string().c_str());
            return false;
        }

        try
        {
            std::filesystem::copy_file(tex.found_path, tex.output_raw_path, std::filesystem::copy_options::overwrite_existing);
        }
        catch (std::filesystem::filesystem_error& ex)
        {
            db_error(engine, "Failed to copy source file from '%s' to '%s': %s", tex.found_path.string().c_str(), tex.output_raw_path.string().c_str(), ex.what());
            return false;
        }

        textures[texture.path] = tex;
        return true;
    };

    for (geometry_material& mat : geometry->get_materials())
    {
        imported_material& imported_mat = materials.emplace_back();
        imported_mat.name = mat.name;
        imported_mat.formatted_name = string_replace(string_lower(mat.name), " ", "_");

        // If material name has a path in it eg. "/mat/something/bleh.png" then strip all of
        // that away, our formatted names should be just the base name.
        size_t slash = imported_mat.formatted_name.find_last_of('/');
        if (slash != std::string::npos)
        {
            imported_mat.formatted_name = imported_mat.formatted_name.substr(slash + 1);
        }

        imported_mat.output_path = output_directory_material_path / (imported_mat.formatted_name + ".yaml");
        imported_mat.vfs_output_path = virtual_file_system::normalize((vfs_output_directory_material_path / (imported_mat.formatted_name + ".yaml")).string().c_str());
        imported_mat.material = mat;

        db_log(engine, "Importing material: %s", imported_mat.output_path.string().c_str());

        // Figure out any textures required.
        if (!add_texture(imported_mat.material.albedo_texture, "color"))
        {
            imported_mat.material.albedo_texture.path = find_texture({ "albedo", "basecolor", "diffuse", "color" });
            if (!imported_mat.material.albedo_texture.path.empty())
            {
                if (!add_texture(imported_mat.material.albedo_texture, "color"))
                {
                    return false;
                }
            }
        }

        if (!add_texture(imported_mat.material.metallic_texture, "metallic"))
        {
            imported_mat.material.metallic_texture.path = find_texture({ "metalness", "metallic" });
            if (!imported_mat.material.metallic_texture.path.empty())
            {
                if (!add_texture(imported_mat.material.metallic_texture, "metallic"))
                {
                    return false;
                }
            }
        }

        if (!add_texture(imported_mat.material.normal_texture, "normal"))
        {
            imported_mat.material.normal_texture.path = find_texture({ "normal" });
            if (!imported_mat.material.normal_texture.path.empty())
            {
                if (!add_texture(imported_mat.material.normal_texture, "normal"))
                {
                    return false;
                }
            }
        }

        if (!add_texture(imported_mat.material.roughness_texture, "roughness"))
        {
            imported_mat.material.roughness_texture.path = find_texture({ "roughness" });
            if (!imported_mat.material.roughness_texture.path.empty())
            {
                if (!add_texture(imported_mat.material.roughness_texture, "roughness"))
                {
                    return false;
                }
            }
        }
    }

    // Write the model template.
    if (import_settings.seperate_submeshes)
    {
        for (geometry_mesh& mesh : geometry->get_meshes())
        {
            std::filesystem::path mesh_name = sanitize_filename(mesh.name.c_str());
            std::filesystem::path mesh_yaml_path = output_directory_path / mesh_name.replace_extension(".yaml");

            if (!write_model_template(mesh_yaml_path.string().c_str(), vfs_output_model_path_normalized.c_str(), materials, mesh.name.c_str(), import_settings))
            {
                db_error(engine, "Failed to write out model asset file: %s", asset_name_with_yaml_extension.string().c_str());
                return false;
            }
        }
    }
    else
    {
        std::filesystem::path mesh_name = asset_name;
        std::filesystem::path mesh_yaml_path = output_directory_path / mesh_name.replace_extension(".yaml");

        if (!write_model_template(mesh_yaml_path.string().c_str(), vfs_output_model_path_normalized.c_str(), materials, "", import_settings))
        {
            db_error(engine, "Failed to write out model asset file: %s", asset_name_with_yaml_extension.string().c_str());
            return false;
        }
    }

    // Write all the material templates.
    for (imported_material& mat : materials)
    {
        if (!write_material_template(mat, textures))
        {
            db_error(engine, "Failed to write out material asset file: %s", mat.vfs_output_path.c_str());
            return false;
        }
    }

    // Write all the texture templates.
    for (auto& iter : textures)
    {
        if (!write_texture_template(iter.second))
        {
            db_error(engine, "Failed to write out texture asset file: %s", iter.second.output_yaml_path.c_str());
            return false;
        }
    }    

    return true;
}

bool model_importer::write_model_template(const char* path, const char* vfs_model_path, const std::vector<imported_material>& materials, const char* source_node_name, const model_importer_settings& import_settings)
{
    std::string output = "";

    output += "# ================================================================================================\n";
    output += "#  workshop\n";
    output += "#  Copyright (C) 2023 Tim Leonard\n";
    output += "# ================================================================================================\n";
    output += "type: model\n";
    output += "version: 1\n";
    output += "\n";
    output += "source: " + std::string(vfs_model_path) + "\n";
    if (source_node_name[0] != '\0')
    {
        output += "source_node: " + std::string(source_node_name) + "\n";
    }
    output += "\n";
    output += string_format("recalculate_origin: %s\n", import_settings.recalculate_origin ? "true" : "false");
    output += string_format("merge_submeshes: %s\n", !import_settings.seperate_submeshes ? "true" : "false");
    output += string_format("scale: [ %.2f, %.2f, %.2f ]\n", import_settings.scale.x, import_settings.scale.y, import_settings.scale.z);
    output += "\n";
    output += "materials:\n";
    for (const imported_material& mat : materials)
    {
        output += string_format("  \"%s\": \"%s\"\n", mat.name.c_str(), mat.vfs_output_path.c_str());
    }
    
    if (!write_all_text(path, output))
    {
        return false;
    }

    return true;
}

bool model_importer::write_material_template(imported_material& material, const std::unordered_map<std::string, imported_texture>& textures)
{
    std::string output = "";

    output += "# ================================================================================================\n";
    output += "#  workshop\n";
    output += "#  Copyright (C) 2023 Tim Leonard\n";
    output += "# ================================================================================================\n";
    output += "type: material\n";
    output += "version: 1\n";
    output += "\n";
    output += "domain: opaque\n";
    output += "\n";

    if (material.material.albedo_texture.path.empty() &&
        material.material.albedo_texture.path.empty() &&
        material.material.albedo_texture.path.empty() &&
        material.material.albedo_texture.path.empty())
    {
        output += "textures: {}\n";
    }
    else
    {
        output += "textures:\n";

        auto add_texture = [&output, &textures](const char* name, geometry_texture& tex) mutable {
            if (tex.path.empty())
            {
                return;
            }

            auto iter = textures.find(tex.path);
            output += string_format("  %s: \"%s\"\n", name, iter->second.vfs_output_yaml_path.c_str());
        };

        add_texture("albedo_texture", material.material.albedo_texture);
        add_texture("normal_texture", material.material.normal_texture);
        add_texture("metallic_texture", material.material.metallic_texture);
        add_texture("roughness_texture", material.material.roughness_texture);
    }

    if (!write_all_text(material.output_path, output))
    {
        return false;
    }

    return true;
}

bool model_importer::write_texture_template(imported_texture& texture)
{
    std::string output = "";

    output += "# ================================================================================================\n";
    output += "#  workshop\n";
    output += "#  Copyright (C) 2023 Tim Leonard\n";
    output += "# ================================================================================================\n";
    output += "type: texture\n";
    output += "version: 1\n";
    output += "\n";
    output += "group: world\n";
    output += "usage: " + texture.usage + "\n";
    output += "\n";
    output += "faces:\n";
    output += "  - " + texture.vfs_output_raw_path + "\n";

    if (!write_all_text(texture.output_yaml_path, output))
    {
        return false;
    }

    return true;
}

}; // namespace ws

