// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/texture/texture_loader.h"
#include "workshop.renderer/assets/texture/texture.h"
#include "workshop.assets/asset_cache.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/drawing/pixmap.h"

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_texture_compiler.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

namespace ws {

namespace {

constexpr const char* k_asset_descriptor_type = "texture";
constexpr size_t k_asset_descriptor_minimum_version = 1;
constexpr size_t k_asset_descriptor_current_version = 1;

// Bump if compiled format ever changes.
constexpr size_t k_asset_compiled_version = 2;

};

template<>
inline void stream_serialize(stream& out, texture::face& face)
{
    stream_serialize(out, face.file);
    stream_serialize_list(out, face.mips);
}

texture_loader::texture_loader(ri_interface& instance, renderer& renderer)
    : m_ri_interface(instance)
    , m_renderer(renderer)
{
}

const std::type_info& texture_loader::get_type()
{
    return typeid(texture);
}

asset* texture_loader::get_default_asset()
{
    return nullptr;
}

asset* texture_loader::load(const char* path)
{
    texture* asset = new texture(m_ri_interface, m_renderer);
    if (!serialize(path, *asset, false))
    {
        delete asset;
        return nullptr;
    }
    return asset;
}

void texture_loader::unload(asset* instance)
{
    delete instance;
}

bool texture_loader::save(const char* path, texture& asset)
{
    return serialize(path, asset, true);
}

bool texture_loader::serialize(const char* path, texture& asset, bool isSaving)
{
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, isSaving);
    if (!stream)
    {
        db_error(asset, "[%s] Failed to open stream to save asset.", path);
        return false;
    }

    if (!isSaving)
    {
        asset.header.type = k_asset_descriptor_type;
        asset.header.version = k_asset_compiled_version;
        asset.name = path;
    }

    if (!serialize_header(*stream, asset.header, path))
    {
        return false;
    }

    stream_serialize_enum(*stream, asset.usage);
    stream_serialize_enum(*stream, asset.dimensions);
    stream_serialize_enum(*stream, asset.format);
    stream_serialize(*stream, asset.width);
    stream_serialize(*stream, asset.height);
    stream_serialize(*stream, asset.depth);
    stream_serialize(*stream, asset.mipmapped);
    stream_serialize(*stream, asset.mip_levels);
    stream_serialize_list(*stream, asset.data);

    return true;
}

bool texture_loader::load_face(const char* path, const char* face_path, texture& asset)
{
    // Load the face from.
    texture::face& face = asset.faces.emplace_back();
    face.file = face_path;
    face.mips.emplace_back(pixmap::load(face_path));

    if (!face.mips[0])
    {
        db_error(asset, "[%s] Failed to load pixmap from referenced file: %s", path, face_path);
        return false;
    }

    return true;
}

bool texture_loader::parse_faces(const char* path, YAML::Node& node, texture& asset)
{
    YAML::Node this_node = node["faces"];

    if (!this_node.IsDefined())
    {
        return true;
    }

    if (this_node.Type() != YAML::NodeType::Sequence)
    {
        db_error(asset, "[%s] faces node is invalid data type.", path);
        return false;
    }

    for (auto iter = this_node.begin(); iter != this_node.end(); iter++)
    {
        if (!iter->IsScalar())
        {
            db_error(asset, "[%s] face value was not scalar value.", path);
            return false;
        }
        else
        {
            std::string value = iter->as<std::string>();

            asset.header.add_dependency(value.c_str());

            if (!load_face(path, value.c_str(), asset))
            {
                return false;
            }
        }
    }

    return true;
}

bool texture_loader::parse_properties(const char* path, YAML::Node& node, texture& asset)
{
    if (!parse_property(path, "usage", node["usage"], asset.usage, false))
    {
        return false;
    }

    if (!parse_property(path, "dimensions", node["dimensions"], asset.dimensions, false))
    {
        return false;
    }

    if (!parse_property(path, "format", node["format"], asset.format, false))
    {
        return false;
    }

    if (!parse_property(path, "width", node["width"], asset.width, false))
    {
        return false;
    }

    if (!parse_property(path, "height", node["height"], asset.height, false))
    {
        return false;
    }

    if (!parse_property(path, "depth", node["depth"], asset.depth, false))
    {
        return false;
    }

    if (!parse_property(path, "mipmapped", node["mipmapped"], asset.mipmapped, false))
    {
        return false;
    }

    return true;
}

bool texture_loader::parse_file(const char* path, texture& asset)
{
    db_verbose(asset, "[%s] Parsing file", path);

    YAML::Node node;
    if (!load_asset_descriptor(path, node, k_asset_descriptor_type, k_asset_descriptor_minimum_version, k_asset_descriptor_current_version))
    {
        return false;
    }

    if (!parse_properties(path, node, asset))
    {
        return false;
    }

    if (!parse_faces(path, node, asset))
    {
        return false;
    }

    return true;
}

bool texture_loader::infer_properties(const char* path, texture& asset)
{
    if (asset.dimensions == ri_texture_dimension::COUNT)
    {
        if (asset.faces.size() == 1)
        {
            if (asset.faces[0].mips[0]->get_height() == 1)
            {
                asset.dimensions = ri_texture_dimension::texture_1d;
            }
            else
            {
                asset.dimensions = ri_texture_dimension::texture_2d;
            }
        }
        else if (asset.faces.size() == 6)
        {
            asset.dimensions = ri_texture_dimension::texture_cube;
        }
        else
        {
            asset.dimensions = ri_texture_dimension::texture_3d;
        }
    }
    if (asset.width == 0)
    {
        for (texture::face& face : asset.faces)
        {
            asset.width = std::max(asset.width, face.mips[0]->get_width());
        }
    }
    if (asset.height == 0)
    {
        for (texture::face& face : asset.faces)
        {
            asset.height = std::max(asset.height, face.mips[0]->get_height());
        }
    }
    if (asset.depth == 0)
    {
        if (asset.dimensions == ri_texture_dimension::texture_3d)
        {
            asset.depth = asset.faces.size();
        }
        else
        {
            asset.depth = 1;
        }
    }
    if (asset.format == pixmap_format::COUNT)
    {
        pixmap& face = *asset.faces[0].mips[0];

        // If more faces than 1 or format is not a standard 32bit value, then use
        // the pixmaps format directly.
        if (asset.faces.size() != 1 || face.get_format() != pixmap_format::R8G8B8A8)
        {
            asset.format = face.get_format();
        }
        else
        {
            if (asset.usage == texture_usage::color)
            {
                bool r_constant = face.is_channel_constant(0, { 0, 0, 0, 255 });
                bool g_constant = face.is_channel_constant(1, { 0, 0, 0, 255 });
                bool b_constant = face.is_channel_constant(2, { 0, 0, 0, 255 });
                bool a_constant = face.is_channel_constant(3, { 0, 0, 0, 255 });

                // If all channels but R are zero, BC4
                if (g_constant && b_constant && a_constant)
                {
                    asset.format = pixmap_format::BC4;
                }
                // If all channels but RG are zero, BC5
                else if (b_constant && a_constant)
                {
                    asset.format = pixmap_format::BC5;
                }
                // If one bit alpha BC1. 
                // Not sure this is really any better than BC7.
                //else if (face.is_channel_one_bit(3))
                //{
                //    asset.format = pixmap_format::BC1;
                //}
                // Otherwise BC7.
                else
                {
                    asset.format = pixmap_format::BC7;
                }
            }
            else if (asset.usage == texture_usage::normal)
            {
                asset.format = pixmap_format::BC5;
            }
            else if (asset.usage == texture_usage::roughness ||
                     asset.usage == texture_usage::metallic ||
                     asset.usage == texture_usage::mask)
            {
                asset.format = pixmap_format::BC4;
            }
            else
            {
                // Fallback to the pixmaps format if we cannot infer anything useful.
                asset.format = face.get_format();
            }
        }
    }

    if (asset.depth != 1)
    {
        if (asset.dimensions == ri_texture_dimension::texture_1d || 
            asset.dimensions == ri_texture_dimension::texture_2d ||
            asset.dimensions == ri_texture_dimension::texture_cube)
        {
            db_error(asset, "[%s] only a depth of 1 is permitted for 1d/2d/cube texture.", path);
            return false;
        }
        else if (asset.dimensions != ri_texture_dimension::texture_3d)
        {
            db_error(asset, "[%s] only a depth of 1 is permitted for non-3d, non-cube textures.", path);
            return false;
        }
    }

    return true;
}

bool texture_loader::perform_resize(const char* path, texture& asset)
{
    for (texture::face& face : asset.faces)
    {
        if (face.mips[0]->get_width() != asset.width ||
            face.mips[0]->get_height() != asset.height)
        {
            face.mips[0] = face.mips[0]->resize(asset.width, asset.height, pixmap_filter::bilinear);
        }
    }

    return true;
}

bool texture_loader::generate_mipchain(const char* path, texture& asset)
{
    pixmap_format_metrics metrics = get_pixmap_format_metrics(asset.format);

    // Minimum size of mip chain is based on minimum block size of encoded format.
    // If we need to encode smaller sizes than this, we should 
    size_t block_size = metrics.block_size;
    if (block_size == 0)
    {
        block_size = 1;
    }

    for (texture::face& face : asset.faces)
    {
        size_t mip_index = 0;
        while (true)
        {
            pixmap& last_mip = *face.mips.back();
            if (last_mip.get_width() == block_size && last_mip.get_height() == block_size)
            {
                break;
            }

            size_t mip_width = std::max(block_size, (last_mip.get_width() / 2));
            size_t mip_height = std::max(block_size, (last_mip.get_height() / 2));

#if 0
            static color s_mip_colors[17] = {
                color::red,
                color::green,
                color::blue,
                color::yellow,
                color::amber,
                color::black,
                color::blue_grey,
                color::brown,
                color::cyan,
                color::deep_orange,
                color::deep_purple,
                color::indigo,
                color::grey,
                color::pure_blue,
                color::pink,
                color::white,
                color::yellow
            };

            std::unique_ptr<pixmap> mip = std::make_unique<pixmap>(mip_width, mip_height, last_mip.get_format());
            for (size_t y = 0; y < mip_height; y++)
            {
                for (size_t x = 0; x < mip_width; x++)
                {
                    mip->set(x, y, s_mip_colors[mip_index]);
                }
            }

            face.mips.emplace_back(std::move(mip));
#else
            face.mips.emplace_back(last_mip.resize(mip_width, mip_height, pixmap_filter::bilinear));
#endif

            mip_index++;
        }
    }

    return true;
}

bool texture_loader::perform_encoding(const char* path, texture& asset)
{
    for (texture::face& face : asset.faces)
    {
        for (size_t i = 0; i < face.mips.size(); i++)    
        {
            if (face.mips[i]->get_format() != asset.format)
            {
                face.mips[i] = face.mips[i]->convert(asset.format);
            }
        }
    }

    return true;
}

bool texture_loader::compile_render_data(const char* path, texture& asset)
{
    std::unique_ptr<ri_texture_compiler> compiler = m_ri_interface.create_texture_compiler();

    std::vector<ri_texture_compiler::texture_face> faces;
    for (texture::face& face : asset.faces)
    {
        ri_texture_compiler::texture_face& output_face = faces.emplace_back();
        output_face.mips.reserve(face.mips.size());

        for (auto& mip : face.mips)
        {
            output_face.mips.push_back(mip.get());
        }
    }

    asset.mip_levels = asset.faces[0].mips.size();

    if (!compiler->compile(
        asset.dimensions,
        asset.width,
        asset.height,
        asset.depth,
        faces,
        asset.data))
    {
        db_error(asset, "[%s] Failed to compile texture.", path);
        return false;
    }

    return true;
}

bool texture_loader::compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config)
{
    texture asset(m_ri_interface, m_renderer);

    // Parse the source YAML file that defines the shader.
    if (!parse_file(input_path, asset))
    {
        return false;
    }

    if (asset.faces.empty())
    {
        db_error(asset, "[%s] no faces of texture were defined.", input_path);
        return false;
    }

    // Infer various properties from the faces that have been defined.
    if (!infer_properties(input_path, asset))
    {
        return false;
    }

    // Resize any mipmaps that don't match our expected width/height.
    if (!perform_resize(input_path, asset))
    {
        return false;
    }

    // Generate a mipchain if required.
    if (asset.mipmapped)
    {
        if (!generate_mipchain(input_path, asset))
        {
            return false;
        }
    }

    // Convert texture into expected format.
    if (!perform_encoding(input_path, asset))
    {
        return false;
    }

    // Generate texture data that can be loaded by render interface.
    if (!compile_render_data(input_path, asset))
    {
        return false;
    }

    // Construct the asset header.
    asset_cache_key compiled_key;
    if (!get_cache_key(input_path, asset_platform, asset_config, compiled_key, asset.header.dependencies))
    {
        db_error(asset, "[%s] Failed to calculate compiled cache key.", input_path);
        return false;
    }
    asset.header.compiled_hash = compiled_key.hash();
    asset.header.type = k_asset_descriptor_type;
    asset.header.version = k_asset_compiled_version;

    // Write binary format to disk.
    if (!save(output_path, asset))
    {
        return false;
    }

    return true;
}

size_t texture_loader::get_compiled_version()
{
    return k_asset_compiled_version;
}

}; // namespace ws

