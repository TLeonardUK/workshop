// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.core/debug/log_handler.h"
#include "workshop.engine/assets/asset_database.h"
#include "workshop.assets/asset_importer.h"

namespace ws {

class editor_log_window;
class asset_manager;
class editor;

// ================================================================================================
//  Window that shows a tree view of all the assets in the game so they can be selected for use.
// ================================================================================================
class editor_assets_window 
    : public editor_window
{
public:
    editor_assets_window(editor* in_editor, asset_manager* ass_manager, asset_database* ass_database);

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:
    void draw_asset_tree_dir(asset_database_entry* entry);
    void draw_asset_tree();

    void draw_asset_text_list();
    void draw_asset_icon_list();

    void import_asset();

    float get_item_size();

    std::vector<asset_database_entry*> get_file_entries();

private:
    asset_manager* m_asset_manager = nullptr;
    asset_database* m_asset_database = nullptr;
    editor* m_editor = nullptr;
    bool m_first_frame = true;

    std::string m_selected_path = "";
    std::string m_selected_file = "";

    int m_current_filter_type = 0;

    float m_zoom_level = 35.0f;

    char m_current_filter[256] = {};

    static inline const float k_show_text_min_zoom = 30.0f;
    static inline const float k_show_list_min_zoom = 1.0f;

    static inline const float k_min_item_width = 32;
    static inline const float k_max_item_width = 256;

    std::unique_ptr<asset_importer_settings> m_import_settings;
    asset_importer* m_importer = nullptr;
    bool m_import_in_progress = false;

    static inline const char* k_import_window_id = "import_popup_window";

    static inline constexpr int k_filter_type_count = 6;
    const char* m_filter_types[k_filter_type_count] = {
        "all",
        "model",
        "texture",
        "material",
        "shader",
        "scene"
    };

};

}; // namespace ws
