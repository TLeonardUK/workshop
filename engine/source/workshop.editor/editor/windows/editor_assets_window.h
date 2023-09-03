// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.core/debug/log_handler.h"
#include "workshop.engine/assets/asset_database.h"

namespace ws {

class editor_log_window;
class asset_manager;

// ================================================================================================
//  Window that shows a tree view of all the assets in the game so they can be selected for use.
// ================================================================================================
class editor_assets_window 
    : public editor_window
{
public:
    editor_assets_window(asset_manager* ass_manager, asset_database* ass_database);

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:
    void draw_asset_tree_dir(asset_database_entry* entry);
    void draw_asset_tree();

    void draw_asset_list();

    void import_asset();

private:
    asset_manager* m_asset_manager = nullptr;
    asset_database* m_asset_database = nullptr;
    bool m_first_frame = true;

    std::string m_selected_path = "";
    std::string m_selected_file = "";

    int m_current_filter_type = 0;

    char m_current_filter[256] = {};

    static inline constexpr int k_filter_type_count = 5;
    const char* m_filter_types[k_filter_type_count] = {
        "all",
        "model",
        "texture",
        "material",
        "shader"
    };

};

}; // namespace ws
