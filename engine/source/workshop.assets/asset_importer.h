// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/hashing/guid.h"
#include "workshop.core/platform/platform.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/utils/traits.h"
#include "workshop.core/reflection/reflect.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

#include <typeinfo>

namespace ws {

class asset;
class stream;

// ================================================================================================
//  Settings used for importing an assert. The reflected values will be displayed to the end-user
//  when trying to import and asset of a given type.
// ================================================================================================
class asset_importer_settings
{
public:
    virtual ~asset_importer_settings() {};

public:
    BEGIN_REFLECT(asset_importer_settings, "Asset Import Settings", reflect_no_parent, reflect_class_flags::none)
    END_REFLECT()

};

// ================================================================================================
//  The base class for a asset importers.
// 
//  Asset importers are resonsible for taking a source file (like an fbx/etc) and copying to the 
//  data folder and building all the asset files to describe it, that can then be loaded by the
//  asset_loader classes.
// 
//  These importers typically have a one-to-one mapping with the relevant loader, but there is
//  no requirement for this. You can have an importer that for example imports a scene file 
//  from an art package and generates all the material/texture/shader files for everything
//  contained within it.
// ================================================================================================
class asset_importer
{
public:

    // Gets the source file extensions that can be imported as this asset type.
    virtual std::vector<std::string> get_supported_extensions() = 0;

    // Gets the name of the file type being imported. This is used as a description 
    // in file dialogs. eg. "Image Files"
    virtual std::string get_file_type_description() = 0;

    // Creates a settings instance for importing asset.
    virtual std::unique_ptr<asset_importer_settings> create_import_settings() = 0;

    // Imports a source file for this asset types and sets up its yaml asset file.
    // source_path is the actual source file (fbx/png/etc).
    // output_path is the directory in the virtual file system we want to place the imported asset files in.
    virtual bool import(const char* source_path, const char* output_path, const asset_importer_settings& settings) = 0;

};

}; // namespace workshop
