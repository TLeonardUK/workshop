// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.editor/editor/utils/property_list.h"
#include "workshop.assets/asset_importer.h"

#include <string>

namespace ws {

class engine;

// ================================================================================================
//  Window that pops ups to show settings for importing a model.
// ================================================================================================
class editor_import_asset_popup
    : public editor_window
{
public:
    editor_import_asset_popup(engine* in_engine);

    void set_import_settings(asset_importer* importer, const std::string& path, const std::string& output_path);

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:
    asset_importer* m_importer;
    std::string m_path;
    std::string m_output_path;

    std::unique_ptr<asset_importer_settings> m_import_settings;
    std::unique_ptr<property_list> m_property_list;

    engine* m_engine;

    bool m_was_open = false;

};

}; // namespace ws
