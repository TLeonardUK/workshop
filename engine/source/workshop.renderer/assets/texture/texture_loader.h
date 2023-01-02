// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset_loader.h"
#include "workshop.renderer/assets/texture/texture.h"

namespace ws {

class asset;
class texture;
class render_interface;
class ri_interface;
class renderer;

// ================================================================================================
//  Loads texture files.
// ================================================================================================
class texture_loader : public asset_loader
{
public:
    texture_loader(ri_interface& instance, renderer& renderer);

    virtual const std::type_info& get_type() override;
    virtual asset* get_default_asset() override;
    virtual asset* load(const char* path) override;
    virtual void unload(asset* instance) override;
    virtual bool compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config) override;
    virtual size_t get_compiled_version() override;

private:
    bool serialize(const char* path, texture& asset, bool isSaving);

    bool save(const char* path, texture& asset);

    bool parse_faces(const char* path, YAML::Node& node, texture& asset);

    bool load_face(const char* path, const char* face_path, texture& asset);

    bool parse_file(const char* path, texture& asset);

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;

};

}; // namespace workshop
