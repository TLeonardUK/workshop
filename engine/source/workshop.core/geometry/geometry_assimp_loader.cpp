// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/geometry/geometry_assimp_loader.h"
#include "workshop.core/geometry/geometry.h"
#include "workshop.core/debug/log.h"
#include "workshop.core/math/triangle.h"
#include "workshop.core/math/math.h"

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
        std::vector<uint32_t> indices;
    };

    size_t uv_channel_count = 0;
    size_t color_channel_count = 0;

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
bool process_mesh(aiMesh* node, const aiScene* scene, import_context& output, matrix4 transform, const geometry_load_settings& settings)
{
    // Skip mesh if its being filtered out.
    if (!settings.only_node.empty() && node->mName.C_Str() != settings.only_node)
    {
        return true;
    }

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
            vector3 pos = vector3(node->mVertices[i].x, node->mVertices[i].y, node->mVertices[i].z) * transform;
            output.positions.push_back(pos);
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
            vector3 normal = transform.transform_direction(vector3(node->mNormals[i].x, node->mNormals[i].y, node->mNormals[i].z));

            output.normals.push_back(normal);
        }
    }
    else
    {
        db_warning(asset, "Mesh contains no normal vertex stream, using dummy data");

        output.normals.reserve(output.normals.size() + node->mNumVertices);
        for (size_t i = 0; i < node->mNumVertices; i++)
        {
            output.normals.push_back(vector3::up);
        }
    }

    if (node->HasTangentsAndBitangents())
    {
        output.tangents.reserve(output.tangents.size() + node->mNumVertices);
        output.bitangents.reserve(output.bitangents.size() + node->mNumVertices);
        for (size_t i = 0; i < node->mNumVertices; i++)
        {
            vector3 tangent = transform.transform_direction(vector3(node->mTangents[i].x, node->mTangents[i].y, node->mTangents[i].z));
            vector3 bitangent = transform.transform_direction(vector3(node->mBitangents[i].x, node->mBitangents[i].y, node->mBitangents[i].z));

            output.tangents.push_back(tangent);
            output.bitangents.push_back(bitangent);
        }
    }
    else
    {
        db_warning(asset, "Mesh '%s' contains no tangent / bitangent vertex stream, using dummy data.", node->mName.C_Str());        
        
        output.tangents.reserve(output.tangents.size() + node->mNumVertices);
        output.bitangents.reserve(output.bitangents.size() + node->mNumVertices);
        for (size_t i = 0; i < node->mNumVertices; i++)
        {
            output.tangents.push_back(vector3::up);
            output.bitangents.push_back(vector3::up);
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

    if (node->GetNumUVChannels() < output.uvs.size())
    {
        for (size_t j = node->GetNumUVChannels(); j < output.uvs.size(); j++)
        {
            std::vector<vector2>& uvs = output.uvs[j];
            uvs.reserve(uvs.size() + node->mNumVertices);

            for (size_t i = 0; i < node->mNumVertices; i++)
            {
                uvs.push_back({ 0.0f, 0.0f });
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

    if (node->GetNumColorChannels() < output.colors.size())
    {
        for (size_t j = node->GetNumColorChannels(); j < output.colors.size(); j++)
        {
            std::vector<vector4>& colors = output.colors[j];
            colors.reserve(colors.size() + node->mNumVertices);

            for (size_t i = 0; i < node->mNumVertices; i++)
            {
                colors.push_back({ 1.0f, 1.0f, 1.0f, 1.0f });
            }
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
        mesh.indices.push_back((uint32_t)(start_vertex_index + face.mIndices[0]));
        mesh.indices.push_back((uint32_t)(start_vertex_index + face.mIndices[1]));
        mesh.indices.push_back((uint32_t)(start_vertex_index + face.mIndices[2]));
    }

    return true;
}

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
bool walk_scene(aiNode* node, const aiScene* scene, import_context& output, matrix4 transform, const geometry_load_settings& settings)
{
    matrix4 node_transform = matrix4(
        node->mTransformation.a1, node->mTransformation.a2, node->mTransformation.a3, node->mTransformation.a4,
        node->mTransformation.b1, node->mTransformation.b2, node->mTransformation.b3, node->mTransformation.b4,
        node->mTransformation.c1, node->mTransformation.c2, node->mTransformation.c3, node->mTransformation.c4,
        node->mTransformation.d1, node->mTransformation.d2, node->mTransformation.d3, node->mTransformation.d4
    ) * transform;

    for (size_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        if (!process_mesh(mesh, scene, output, node_transform, settings))
        {
            return false;
        }
    }

    for (size_t i = 0; i < node->mNumChildren; i++)
    {
        if (!walk_scene(node->mChildren[i], scene, output, node_transform, settings))
        {
            return false;
        }
    }

    return true;
}

#pragma optimize("", off)

// Imports all materials from a scene into the context.
bool import_materials(const aiScene* scene, import_context& context)
{
    // Create list of materials that are actively in use.
    std::unordered_map<size_t, size_t> original_index_to_new_index;
    size_t next_new_index = 0;
    
    // Go through all materials actively being used by imported meshes.
    for (import_context::mesh& mesh : context.meshes)
    {
        auto iter = original_index_to_new_index.find(mesh.material_index);
        if (iter == original_index_to_new_index.end())
        {
            size_t original_material_index = mesh.material_index;

            original_index_to_new_index[mesh.material_index] = next_new_index;
            mesh.material_index = next_new_index;
            next_new_index++;

            // Add material to output.
            aiMaterial* material = scene->mMaterials[original_material_index];
            if (!process_material(material, scene, context))
            {
                return false;
            }
        }
        else
        {
            mesh.material_index = iter->second;
        }
    }

    return true;
}

// Counts maximum number of uv channels and allocates appropriate space in context.
void allocate_uv_channels(const aiScene* scene, import_context& context)
{
    context.uv_channel_count = 0;
    context.color_channel_count = 0;

    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        context.uv_channel_count = std::max(context.uv_channel_count, (size_t)mesh->GetNumUVChannels());
        context.color_channel_count = std::max(context.color_channel_count, (size_t)mesh->GetNumColorChannels());
    }

    context.uvs.resize(context.uv_channel_count);
    context.colors.resize(context.color_channel_count);
}

// Normalizes the data streams in the import context.
void normalize_streams(const aiScene* scene, import_context& context)
{
    for (vector3& input : context.normals)
    {
        input = input.normalize();
    }
    for (vector3& input : context.tangents)
    {
        input = input.normalize();
    }
    for (vector3& input : context.bitangents)
    {
        input = input.normalize();
    }
}

// Applies a scale to all spatial data streams in the context.
void apply_scale(import_context& context, const vector3& scale)
{
    for (vector3& pos : context.positions)
    {
        pos *= scale;
    }
}

void add_materials_to_geometry(geometry& geo, import_context& context)
{
    for (import_context::material& mat : context.materials)
    {
        size_t index = geo.add_material(mat.name.c_str());

        geometry_material& new_mat = geo.get_materials()[index];
        new_mat.albedo_texture.path = mat.albedo_source;
        new_mat.normal_texture.path = mat.normal_source;
        new_mat.metallic_texture.path = mat.metallic_source;
        new_mat.roughness_texture.path = mat.roughness_source;
    }
}

void add_meshes_to_geometry(geometry& geo, import_context& context, const geometry_load_settings& settings)
{
    for (import_context::mesh& mesh : context.meshes)
    {
        vector3 mesh_bounds_min = vector3(FLT_MAX, FLT_MAX, FLT_MAX);
        vector3 mesh_bounds_max = vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (uint32_t index : mesh.indices)
        {
            vector3& pos = context.positions[index];

            // Calcualte bounds.
            mesh_bounds_min = vector3::min(mesh_bounds_min, pos);
            mesh_bounds_max = vector3::max(mesh_bounds_max, pos);
        }

        double min_texel_area = std::numeric_limits<double>::max();
        double max_texel_area = std::numeric_limits<double>::min();
        double min_world_area = std::numeric_limits<double>::max();
        double max_world_area = std::numeric_limits<double>::min();
        double avg_texel_area = 0.0f;
        double avg_world_area = 0.0f;
        double uv_density = 1.0f;

        // Calculate texel factor of each triangle and use the minimum as our streaming bias.
        if (!context.uvs.empty())
        {
            std::vector<double> triangle_texel_area;
            std::vector<double> triangle_world_area;

            triangle_texel_area.reserve(mesh.indices.size() / 3);
            triangle_world_area.reserve(mesh.indices.size() / 3);

            uv_density = 1.0f;
            bool first_triangle = true;

            for (size_t i = 0; i < mesh.indices.size(); i += 3)
            {
                size_t i0 = mesh.indices[i + 0];
                size_t i1 = mesh.indices[i + 1];
                size_t i2 = mesh.indices[i + 2];

                vector3& a = context.positions[i0];
                vector3& b = context.positions[i1];
                vector3& c = context.positions[i2];

                vector2& a_uv = context.uvs[0][i0];
                vector2& b_uv = context.uvs[0][i1];
                vector2& c_uv = context.uvs[0][i2];

                triangle tri(a, b, c);
                triangle2d tri_uv(a_uv, b_uv, c_uv);

                // Rather than the area, we take the longest side and square it, this gives a better
                // texel density calculation and avoids garbage results from very thin polys.
                //float original_world_area = tri.get_area();
                //float original_texel_area = tri_uv.get_area();
                double world_area = tri.get_longest_side() * tri.get_longest_side();
                double texel_area = tri_uv.get_longest_side() * tri_uv.get_longest_side();

                if (texel_area <= FLT_EPSILON ||
                    world_area <= FLT_EPSILON)
                {
                    continue;
                }

                triangle_texel_area.push_back(texel_area);
                triangle_world_area.push_back(world_area);

                double tri_uv_density = tri_uv.get_longest_side() / tri.get_longest_side();
                if (first_triangle)
                {
                    uv_density = tri_uv_density;
                    first_triangle = false;
                }
                else
                {
                    uv_density = std::max(uv_density, tri_uv_density);
                }
            }

            double texel_area_deviation = math::calculate_standard_deviation<double>(triangle_texel_area.begin(), triangle_texel_area.end());
            double texel_area_mean = math::calculate_mean<double>(triangle_texel_area.begin(), triangle_texel_area.end());
            double texel_area_samples = 0.0f;

            double world_area_deviation = math::calculate_standard_deviation<double>(triangle_world_area.begin(), triangle_world_area.end());
            double world_area_mean = math::calculate_mean<double>(triangle_world_area.begin(), triangle_world_area.end());
            double world_area_samples = 0.0f;

            // Calculate the min/max/avg areas.
            for (double value : triangle_texel_area)
            {
                double distance = math::abs(texel_area_mean - value);
                if (distance > texel_area_deviation)
                {
                    continue;
                }

                min_texel_area = std::min(min_texel_area, value);
                max_texel_area = std::max(max_texel_area, value);
                avg_texel_area += value;
                texel_area_samples += 1.0f;
            }
            for (double value : triangle_world_area)
            {
                double distance = math::abs(world_area_mean - value);
                if (distance > world_area_deviation)
                {
                    continue;
                }

                min_world_area = std::min(min_world_area, value);
                max_world_area = std::max(max_world_area, value);
                avg_world_area += value;
                world_area_samples += 1.0f;
            }

            avg_texel_area /= texel_area_samples;
            avg_world_area /= world_area_samples;
            uv_density = sqrt(uv_density);
        }

        geo.add_mesh(
            mesh.name.c_str(), 
            mesh.material_index, 
            mesh.indices, 
            aabb(mesh_bounds_min, mesh_bounds_max), 
            (float)min_texel_area,
            (float)max_texel_area,
            (float)avg_texel_area,
            (float)min_world_area,
            (float)max_world_area,
            (float)avg_world_area,
            (float)uv_density
        );
    }
}

void calculate_geometry_bounds(geometry& geo, import_context& context)
{
    if (context.positions.empty())
    {
        geo.bounds.min = vector3::zero;
        geo.bounds.max = vector3::zero;
    }
    else
    {
        geo.bounds.min = context.positions[0];
        geo.bounds.max = context.positions[0];

        for (vector3& pos : context.positions)
        {
            geo.bounds.min = vector3::min(geo.bounds.min, pos);
            geo.bounds.max = vector3::max(geo.bounds.max, pos);
        }
    }
}

void add_vertex_streams_to_geometry(geometry& geo, import_context& context)
{
    geo.add_vertex_stream(geometry_vertex_stream_type::position, context.positions);
    geo.add_vertex_stream(geometry_vertex_stream_type::normal, context.normals);
    geo.add_vertex_stream(geometry_vertex_stream_type::tangent, context.tangents);
    geo.add_vertex_stream(geometry_vertex_stream_type::bitangent, context.bitangents);
    for (size_t i = 0; i < context.uvs.size(); i++)
    {
        geo.add_vertex_stream(static_cast<geometry_vertex_stream_type>((int)geometry_vertex_stream_type::uv0 + i), context.uvs[i]);
    }
    for (size_t i = 0; i < context.colors.size(); i++)
    {
        geo.add_vertex_stream(static_cast<geometry_vertex_stream_type>((int)geometry_vertex_stream_type::color0 + i), context.colors[i]);
    }
}

void recalculate_origin(import_context& context)
{
    // If recalculating the origin for each mesh, shift all vertices used by the mesh so
    // the bottom-center is touching zero.

    aabb bounds;

    if (context.positions.empty())
    {
        bounds.min = vector3::zero;
        bounds.max = vector3::zero;
    }
    else
    {
        bounds.min = context.positions[0];
        bounds.max = context.positions[0];

        for (vector3& pos : context.positions)
        {
            bounds.min = vector3::min(bounds.min, pos);
            bounds.max = vector3::max(bounds.max, pos);
        }
    }

    vector3 origin(
        bounds.min.x + ((bounds.max.x - bounds.min.x) * 0.5f),
        bounds.min.y,
        bounds.min.z + ((bounds.max.z - bounds.min.z) * 0.5f)
    );

    for (vector3& pos : context.positions)
    {
        pos -= origin;
    }
}

};

std::unique_ptr<geometry> geometry_assimp_loader::load(const std::vector<char>& buffer, const char* path_hint, const geometry_load_settings& settings)
{
    Assimp::Importer importer;

    int flags = 0;
    if (settings.high_quality)
    {
        flags = (aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);
    }
    else
    {
        flags = (aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded);
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

    allocate_uv_channels(scene, context);
    
    // Import all meshes in scene.
    if (!walk_scene(scene->mRootNode, scene, context, matrix4::identity, settings))
    {
        return nullptr;
    }

    // Import all materials that are in use.
    if (!import_materials(scene, context))
    {
        return nullptr;
    }

    // Normalize all normal values. Some models have these as non-unit vectors which causes issues elsewhere in the engine.
    normalize_streams(scene,  context);

    // Apply scale to spatial data streams..
    apply_scale(context, settings.scale);

    // Recalculate the bounds
    if (settings.recalculate_origin)
    {
        recalculate_origin(context);
    }

    // Create resulting geometry.
    std::unique_ptr<geometry> result = std::make_unique<geometry>();
    add_materials_to_geometry(*result, context);
    add_meshes_to_geometry(*result, context, settings);
    add_vertex_streams_to_geometry(*result, context); 

    // Recalculate the final geometry bounds.
    calculate_geometry_bounds(*result, context);

    return result;
}

bool geometry_assimp_loader::supports_extension(const char* extension)
{
    std::vector<const char*> supported_extensions = {
        ".3d", ".3ds", ".3mf", ".ac", ".amf", ".ase", ".assbin", ".assjson", ".assxml", ".b3d",
        ".blend", ".bvh", ".cob", ".collada", ".csm", ".dxf", ".fbx", ".gltf", ".hmp", ".ifc",
        ".iqm", ".irr", ".irrmesh", ".lwo", ".lws", ".m3d", ".md2", ".md3", ".md5", ".mdc", ".mdl",
        ".mmd", ".ms3d", ".ndo", ".nff", ".obj", ".off", ".ogre", ".opengex", ".pbrt", ".ply", ".q3bsp",
        ".q3d", ".raw", ".sib", ".smd", ".step", ".stl", ".terragen", ".x", ".x3d", ".xgl", ".glb"
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


