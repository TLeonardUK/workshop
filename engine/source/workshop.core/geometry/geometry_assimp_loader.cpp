// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/geometry/geometry_assimp_loader.h"
#include "workshop.core/geometry/geometry.h"
#include "workshop.core/debug/log.h"

#include "thirdparty/assimp/include/assimp/Importer.hpp"
#include "thirdparty/assimp/include/assimp/scene.h"
#include "thirdparty/assimp/include/assimp/mesh.h"
#include "thirdparty/assimp/include/assimp/postprocess.h"

#if 1
#include "workshop.core/filesystem/virtual_file_system.h"
#include <sstream>
#endif

namespace ws {

namespace {

struct import_context
{
    struct material
    {
        std::string name;

        std::string albedo_source;
        std::string metallic_source;
        std::string normal_source;
        std::string roughness_source;
    };

    struct mesh
    {
        std::string name;
        size_t material_index;
        std::vector<size_t> indices;
    };

    std::vector<vector3> positions;
    std::vector<vector3> normals;
    std::vector<vector3> tangents;
    std::vector<vector3> bitangents;
    std::vector<std::vector<vector2>> uvs;
    std::vector<std::vector<vector4>> colors;
    std::vector<material> materials;
    std::vector<mesh> meshes;
};
 
// Imports a mesh node in the assimp scene.
bool process_mesh(aiMesh* node, const aiScene* scene, import_context& output)
{
    size_t start_vertex_index = output.positions.size();

    if (node->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
    {
        db_warning(asset, "Skipping non-triangle primitive data.");
        return true;
    }

    if (node->HasPositions())
    {
        output.positions.reserve(output.positions.size() + node->mNumVertices);
        for (size_t i = 0; i < node->mNumVertices; i++)
        {
            output.positions.push_back({ node->mVertices[i].x, node->mVertices[i].y, node->mVertices[i].z });
        }
    }
    else
    {
        db_warning(asset, "Mesh contains no position vertex stream.");
        return false;
    }

    if (node->HasNormals())
    {
        output.normals.reserve(output.normals.size() + node->mNumVertices);
        for (size_t i = 0; i < node->mNumVertices; i++)
        {
            output.normals.push_back({ node->mNormals[i].x, node->mNormals[i].y, node->mNormals[i].z });
        }
    }
    else
    {
        db_warning(asset, "Mesh contains no normal vertex stream.");
        return false;
    }

    if (node->HasTangentsAndBitangents())
    {
        output.tangents.reserve(output.tangents.size() + node->mNumVertices);
        output.bitangents.reserve(output.bitangents.size() + node->mNumVertices);
        for (size_t i = 0; i < node->mNumVertices; i++)
        {
            output.tangents.push_back({ node->mTangents[i].x, node->mTangents[i].y, node->mTangents[i].z });
            output.bitangents.push_back({ node->mBitangents[i].x, node->mBitangents[i].y, node->mBitangents[i].z });
        }
    }
    else
    {
        db_warning(asset, "Mesh contains no tangent / bitangent vertex stream.");
        return false;
    }

    if (node->GetNumUVChannels() > output.uvs.size())
    {
        size_t original_size = output.uvs.size();

        output.uvs.resize(node->GetNumUVChannels());

        // Add dummy entries for existing entries.
        if (original_size != 0)
        {
            size_t original_vert_count = output.uvs[0].size();

            for (size_t i = original_size; i < output.uvs.size(); i++)
            {
                output.uvs[i].resize(original_vert_count, { 0.0f, 0.0f });
            }
        }
    }

    for (size_t j = 0; j < node->GetNumUVChannels(); j++)
    {
        std::vector<vector2>& uvs = output.uvs[j];
        uvs.reserve(uvs.size() + node->mNumVertices);
        for (size_t i = 0; i < node->mNumVertices; i++)
        {
            const aiVector3D& vert = *(node->mTextureCoords[j] + i);
            uvs.push_back({ vert.x, vert.y });
        }
    }

    if (node->GetNumColorChannels() > output.colors.size())
    {
        size_t original_size = output.colors.size();

        output.colors.resize(node->GetNumColorChannels());

        // Add dummy entries for existing entries.
        if (original_size != 0)
        {
            size_t original_vert_count = output.colors[0].size();

            for (size_t i = original_size; i < output.colors.size(); i++)
            {
                output.colors[i].resize(original_vert_count, { 0.0f, 0.0f, 0.0f, 0.0f });
            }
        }
    }

    for (size_t j = 0; j < node->GetNumColorChannels(); j++)
    {
        std::vector<vector4>& colors = output.colors[j];
        colors.reserve(colors.size() + node->mNumVertices);
        for (size_t i = 0; i < node->mNumVertices; i++)
        {
            const aiColor4D& vert = *(node->mColors[j] + i);
            colors.push_back({ vert.r, vert.g, vert.b, vert.a });
        }
    }

    // Build a mesh for this node.    
    import_context::mesh& mesh = output.meshes.emplace_back();
    mesh.name = node->mName.C_Str();
    mesh.material_index = node->mMaterialIndex;
    mesh.indices.reserve(mesh.indices.size() + (node->mNumFaces * 3));
    for (size_t i = 0; i < node->mNumFaces; i++)
    {
        const aiFace& face = node->mFaces[i];
        mesh.indices.push_back(start_vertex_index + face.mIndices[0]);
        mesh.indices.push_back(start_vertex_index + face.mIndices[1]);
        mesh.indices.push_back(start_vertex_index + face.mIndices[2]);
    }

    return true;
}

#pragma optimize("", off)

// Imports a material node in the assimp scene.
bool process_material(aiMaterial* node, const aiScene* scene, import_context& output)
{
    import_context::material mat;
    mat.name = node->GetName().C_Str();

    aiString texture_path;

    if (node->GetTexture(aiTextureType_BASE_COLOR, 0, &texture_path) == AI_SUCCESS)
    {
        mat.albedo_source = texture_path.C_Str();
    }
    if (node->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path) == AI_SUCCESS)
    {
        mat.albedo_source = texture_path.C_Str();
    }
    if (node->GetTexture(aiTextureType_NORMALS, 0, &texture_path) == AI_SUCCESS)
    {
        mat.normal_source = texture_path.C_Str();
    }
    if (node->GetTexture(aiTextureType_METALNESS, 0, &texture_path) == AI_SUCCESS)
    {
        mat.metallic_source = texture_path.C_Str();
    }
    if (node->GetTexture(aiTextureType_SHININESS, 0, &texture_path) == AI_SUCCESS)
    {
        mat.roughness_source = texture_path.C_Str();
    }
    if (node->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texture_path) == AI_SUCCESS)
    {
        mat.roughness_source = texture_path.C_Str();
    }

    output.materials.push_back(mat);
    return true;
}

// Walks the assimp scene to extract our output.
bool walk_scene(aiNode* node, const aiScene* scene, import_context& output)
{
    for (size_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        if (!process_mesh(mesh, scene, output))
        {
            return false;
        }
    }

    for (size_t i = 0; i < node->mNumChildren; i++)
    {
        if (!walk_scene(node->mChildren[i], scene, output))
        {
            return false;
        }
    }

    return true;
}

};

std::unique_ptr<geometry> geometry_assimp_loader::load(const std::vector<char>& buffer, const char* path_hint, const vector3& scale, bool high_quality)
{
    Assimp::Importer importer;

    int flags = 0;
    if (high_quality)
    {
        flags = (aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices | aiProcess_ConvertToLeftHanded);
    }
    else
    {
        flags = (aiProcessPreset_TargetRealtime_Fast | aiProcess_PreTransformVertices | aiProcess_ConvertToLeftHanded);
    }

    flags &= ~aiProcess_RemoveRedundantMaterials;

    const aiScene* scene = importer.ReadFileFromMemory(
        buffer.data(), 
        buffer.size(), 
        flags,
        path_hint);
    
    if (scene == nullptr)
    {
        db_warning(asset, "Failed to load geometry with error: %s", importer.GetErrorString());
        return nullptr;
    }

    import_context context;

    // Import all materials.
    for (size_t i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* material = scene->mMaterials[i];
        if (!process_material(material, scene, context))
        {
            return nullptr;
        }
    }

    // Import all meshes in scene.
    if (!walk_scene(scene->mRootNode, scene, context))
    {
        return nullptr;
    }

    // Import all skeletons.
    // TODO

    // Import all animations.
    // TODO

    // Create resulting geometry.
    std::unique_ptr<geometry> result = std::make_unique<geometry>();

    // Scale all positions.
    for (vector3& pos : context.positions)
    {
        pos *= scale;
    }

    // Calculate geometry bounds.
    if (context.positions.empty())
    {
        result->bounds.min = vector3::zero;
        result->bounds.max = vector3::zero;
    }
    else
    {
        result->bounds.min = context.positions[0];
        result->bounds.max = context.positions[0];

        for (vector3& pos : context.positions)
        {
            result->bounds.min = vector3::min(result->bounds.min, pos);
            result->bounds.max = vector3::max(result->bounds.max, pos);
        }
    }

    // Insert all the materials into the geometry.
    for (import_context::material& mat : context.materials)
    {
        size_t index = result->add_material(mat.name.c_str());

        geometry_material& new_mat = result->get_materials()[index];
        new_mat.albedo_texture.path = mat.albedo_source;
        new_mat.normal_texture.path = mat.normal_source;
        new_mat.metallic_texture.path = mat.metallic_source;
        new_mat.roughness_texture.path = mat.roughness_source;
    }

    // Insert meshes into the geometry.
    for (import_context::mesh& mesh : context.meshes)
    {
        vector3 mesh_bounds_min = vector3::zero;
        vector3 mesh_bounds_max = vector3::zero;

        for (size_t index : mesh.indices)
        {
            vector3& pos = context.positions[index];

            mesh_bounds_min = vector3::min(mesh_bounds_min, pos);
            mesh_bounds_max = vector3::max(mesh_bounds_max, pos);
        }

        result->add_mesh(mesh.name.c_str(), mesh.material_index, mesh.indices, aabb(mesh_bounds_min, mesh_bounds_max));
    }

    // Insert all the vertex streams.
    result->add_vertex_stream("position", context.positions);
    result->add_vertex_stream("normal", context.normals);
    result->add_vertex_stream("tangent", context.tangents);
    result->add_vertex_stream("bitangent", context.bitangents);
    for (size_t i = 0; i < context.uvs.size(); i++)
    {
        result->add_vertex_stream(string_format("uv%zi", i).c_str(), context.uvs[i]);
    }
    for (size_t i = 0; i < context.colors.size(); i++)
    {
        result->add_vertex_stream(string_format("color%zi", i).c_str(), context.colors[i]);
    }

#if 0
    // DEBUG DEBUG DEBUG DEBUG
    for (import_context::material& mat : context.materials)
    {
        if (mat.indices.empty())
        {
            continue;
        }

        std::string path = string_format("data:models/test_scenes/bistro/materials/%s.yaml", mat.name.c_str());
        std::string albedo_path = string_format("data:models/test_scenes/bistro/textures/%s_BaseColor.yaml", mat.name.c_str());
        std::string normal_path = string_format("data:models/test_scenes/bistro/textures/%s_Normal.yaml", mat.name.c_str());
        std::string roughness_path = string_format("data:models/test_scenes/bistro/textures/%s_Roughness.yaml", mat.name.c_str());
        std::string metal_path = string_format("data:models/test_scenes/bistro/textures/%s_Metallic.yaml", mat.name.c_str());

        std::string roughness_png = string_format("data:models/test_scenes/bistro/textures/%s_Specular.dds", mat.name.c_str());
        std::string metal_png = string_format("data:models/test_scenes/bistro/textures/%s_Specular.dds", mat.name.c_str());

        bool roughness_exists = virtual_file_system::get().exists(roughness_png.c_str());
        bool metal_exists = virtual_file_system::get().exists(metal_png.c_str());

        db_log(renderer, "\t\"%s\": \"\"", mat.name.c_str(), path.c_str());

        std::stringstream ss;
        ss << "# ================================================================================================" << std::endl;
        ss << "#  workshop" << std::endl;
        ss << "#  Copyright (C) 2022 Tim Leonard" << std::endl;
        ss << "# ================================================================================================" << std::endl;
        ss << "type: material" << std::endl;
        ss << "version: 1" << std::endl;
        ss << "" << std::endl;
        ss << "domain: opaque" << std::endl;
        ss << "" << std::endl;
        ss << "textures:" << std::endl;
        ss << " albedo_texture:     " << albedo_path << std::endl;
        ss << " normal_texture:     " << normal_path << std::endl;
        if (metal_exists)
        {
            ss << " metallic_texture:   " << metal_path << std::endl;
        }
        if (roughness_exists)
        {
            ss << " roughness_texture:  " << roughness_path << std::endl;
        }

        std::string str = ss.str();
        std::unique_ptr<stream> s = virtual_file_system::get().open(path.c_str(), true);
        s->write(str.c_str(), str.size());

        auto emit_texture = [](const char* name, const char* path, const char* type) {

            std::stringstream ss;
            ss << "# ================================================================================================" << std::endl;
            ss << "#  workshop" << std::endl;
            ss << "#  Copyright (C) 2022 Tim Leonard" << std::endl;
            ss << "# ================================================================================================" << std::endl;
            ss << "type: texture" << std::endl;
            ss << "version: 1" << std::endl;
            ss << "" << std::endl;
            ss << "group: world" << std::endl;
            ss << "" << std::endl;
            ss << "usage: " << type << std::endl;
            ss << "" << std::endl;
            if (strcmp(type, "roughness") == 0)
            {
                ss << "swizzle: gggg" << std::endl;
                ss << "" << std::endl;
            }
            else if (strcmp(type, "metallic") == 0)
            {
                ss << "swizzle: bbbb" << std::endl;
                ss << "" << std::endl;
            }
            ss << "faces:" << std::endl;
            ss << " - data:models/test_scenes/bistro/textures/" << name << ".dds" << std::endl;

            std::string str = ss.str();
            std::unique_ptr<stream> s = virtual_file_system::get().open(path, true);
            s->write(str.c_str(), str.size());

        };

        //emit_texture(string_format("%s_BaseColor", mat.name.c_str()).c_str(), albedo_path.c_str(), "color");
        //emit_texture(string_format("%s_Normal", mat.name.c_str()).c_str(), normal_path.c_str(), "normal");
        if (roughness_exists)
        {
            emit_texture(string_format("%s_Specular", mat.name.c_str()).c_str(), roughness_path.c_str(), "roughness");
        }
        if (metal_exists)
        {
            emit_texture(string_format("%s_Specular", mat.name.c_str()).c_str(), metal_path.c_str(), "metallic");
        }
    }
    // DEBUG DEBUG DEBUG DEBUG
#endif

    return result;
}

bool geometry_assimp_loader::supports_extension(const char* extension)
{
    std::vector<const char*> supported_extensions = {
        ".3d", ".3ds", ".3mf", ".ac", ".amf", ".ase", ".assbin", ".assjson", ".assxml", ".b3d",
        ".blend", ".bvh", ".cob", ".collada", ".csm", ".dxf", ".fbx", ".gltf", ".hmp", ".ifc",
        ".iqm", ".irr", ".irrmesh", ".lwo", ".lws", ".m3d", ".md2", ".md3", ".md5", ".mdc", ".mdl",
        ".mmd", ".ms3d", ".ndo", ".nff", ".obj", ".off", ".ogre", ".opengex", ".pbrt", ".ply", ".q3bsp",
        ".q3d", ".raw", ".sib", ".smd", ".step", ".stl", ".terragen", ".x", ".x3d", ".xgl"
    };

    for (const char* ext : supported_extensions)
    {
        if (_stricmp(extension, ext) == 0)
        {
            return true;
        }
    }

    return false;
}

}; // namespace workshop


