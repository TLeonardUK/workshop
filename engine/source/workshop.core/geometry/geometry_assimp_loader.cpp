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

#pragma optimize("", off)

namespace ws {

namespace {

struct import_context
{
    std::vector<vector3> positions;
    std::vector<vector3> normals;
    std::vector<vector3> tangents;
    std::vector<vector3> bitangents;
    std::vector<std::vector<vector3>> uvs;
    std::vector<std::vector<vector4>> colors;
    std::vector<size_t> materials;
    std::vector<size_t> indices;

    std::vector<std::string> material_names;
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
                output.uvs[i].resize(original_vert_count, { 0.0f, 0.0f, 0.0f });
            }
        }
    }

    for (size_t j = 0; j < node->GetNumUVChannels(); j++)
    {
        std::vector<vector3>& uvs = output.uvs[j];
        uvs.reserve(uvs.size() + node->mNumVertices);
        for (size_t i = 0; i < node->mNumVertices; i++)
        {
            const aiVector3D& vert = *(node->mTextureCoords[j] + i);
            uvs.push_back({ vert.x, vert.y, vert.z });
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

    output.materials.reserve(output.materials.size() + node->mNumVertices);
    for (size_t i = 0; i < node->mNumVertices; i++)
    {
        output.materials.push_back(node->mMaterialIndex);
    }

    output.indices.reserve(output.indices.size() + (node->mNumFaces * 3));
    for (size_t i = 0; i < node->mNumFaces; i++)
    {
        const aiFace& face = node->mFaces[i];
        output.indices.push_back(start_vertex_index + face.mIndices[0]);
        output.indices.push_back(start_vertex_index + face.mIndices[1]);
        output.indices.push_back(start_vertex_index + face.mIndices[2]);
    }

    return true;
}

// Imports a material node in the assimp scene.
bool process_material(aiMaterial* node, const aiScene* scene, import_context& output)
{
    output.material_names.push_back(node->GetName().C_Str());
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

std::unique_ptr<geometry> geometry_assimp_loader::load(const std::vector<char>& buffer, const char* path_hint)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFileFromMemory(
        buffer.data(), 
        buffer.size(), 
        (aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices | aiProcess_ConvertToLeftHanded) & ~aiProcess_RemoveRedundantMaterials,
        path_hint);
    
    if (scene == nullptr)
    {
        db_warning(asset, "Failed to load geometry with error: %s", importer.GetErrorString());
        return nullptr;
    }

    import_context context;

    // Import all meshes in scene.
    if (!walk_scene(scene->mRootNode, scene, context))
    {
        return nullptr;
    }

    // Import all materials.
    for (size_t i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* material = scene->mMaterials[i];
        if (!process_material(material, scene, context))
        {
            return nullptr;
        }
    }

    // Import all skeletons.
    // TODO

    // Import all animations.
    // TODO

    // Create resulting geometry.
    std::unique_ptr<geometry> result = std::make_unique<geometry>();

    // Insert all the materials into the geometry.
    for (size_t i = 0; i < context.material_names.size(); i++)
    {
        std::vector<size_t> indices;
        indices.reserve(context.materials.size());
        for (size_t j = 0; j < context.materials.size(); j++)
        {
            if (context.materials[j] == i)
            {
                indices.push_back(j);
            }
        }

        result->add_material(context.material_names[i].c_str(), indices);
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
