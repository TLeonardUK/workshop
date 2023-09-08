# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

# The editor library contains all the basic ui constructs required to display
# editor releated functionality.

project(workshop.editor C CXX)

SET(SOURCES
    "editor/editor.cpp"
    "editor/editor.h"
    "editor/editor_main_menu.cpp"
    "editor/editor_main_menu.h"
    "editor/editor_window.cpp"
    "editor/editor_window.h"
    "editor/editor_undo_stack.cpp"
    "editor/editor_undo_stack.h"
    "editor/editor_clipboard.cpp"
    "editor/editor_clipboard.h"
    
    "editor/windows/editor_assets_window.cpp"
    "editor/windows/editor_assets_window.h"
    "editor/windows/editor_loading_window.cpp"
    "editor/windows/editor_loading_window.h"
    "editor/windows/editor_log_window.cpp"
    "editor/windows/editor_log_window.h"
    "editor/windows/editor_memory_window.cpp"
    "editor/windows/editor_memory_window.h"
    "editor/windows/editor_properties_window.cpp"
    "editor/windows/editor_properties_window.h"
    "editor/windows/editor_scene_tree_window.cpp"
    "editor/windows/editor_scene_tree_window.h"
    
    "editor/windows/popups/editor_progress_popup.cpp"
    "editor/windows/popups/editor_progress_popup.h"

    "editor/transactions/editor_transaction_change_object_transform.h"
    "editor/transactions/editor_transaction_change_object_transform.cpp"
    "editor/transactions/editor_transaction_change_selected_objects.h"
    "editor/transactions/editor_transaction_change_selected_objects.cpp"
    "editor/transactions/editor_transaction_create_objects.h"
    "editor/transactions/editor_transaction_create_objects.cpp"
    "editor/transactions/editor_transaction_delete_objects.h"
    "editor/transactions/editor_transaction_delete_objects.cpp"
    "editor/transactions/editor_transaction_create_component.h"
    "editor/transactions/editor_transaction_create_component.cpp"
    "editor/transactions/editor_transaction_delete_component.h"
    "editor/transactions/editor_transaction_delete_component.cpp"
    "editor/transactions/editor_transaction_modify_component.h"
    "editor/transactions/editor_transaction_modify_component.cpp"

    "editor/clipboard/editor_object_clipboard_entry.cpp"
    "editor/clipboard/editor_object_clipboard_entry.h"
    
    "editor/utils/allocation_tree.cpp"
    "editor/utils/allocation_tree.h"
    "editor/utils/property_list.cpp"
    "editor/utils/property_list.h"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME} 
    workshop.core
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier2/editor")
