// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/utils/frame_time.h"

#include "thirdparty/imgui/imgui.h"

namespace ws {

class renderer;
class render_imgui_manager;
class input_interface;

// ================================================================================================
//  
// ================================================================================================
class imgui_scope
{
public:
    imgui_scope(render_imgui_manager& manager, const char* name);
    ~imgui_scope();

private:
    render_imgui_manager& m_manager;

};

// ================================================================================================
//  Handles management of imgui.
// ================================================================================================
class render_imgui_manager
{
public:

    render_imgui_manager(renderer& renderer, input_interface& input);

    // Registers all the steps required to initialize the system.
    void register_init(init_list& list);

    // Completes the imgui frame and resets ready for the next frame.
    void step(frame_time& time);

private:

    struct context
    {
        bool free;
        ImGuiContext* context;
    };

    static const char* get_clipboard_text_callback(void* userdata);
    static void set_clipboard_text_callback(void* userdata, const char* text);

    bool create_imgui(init_list& list);
    bool destroy_imgui();

    void enter_scope(const char* name);
    void leave_scope();

    context* create_context(const char* name);
    void free_context(context* context);

    void start_context_frame(context* context);

    void create_font_resources();

private:
    friend class imgui_scope;

    std::recursive_mutex m_scope_mutex;

    std::string m_clipboard_text;

    renderer& m_renderer;
    input_interface& m_input;

    std::vector<std::unique_ptr<context>> m_contexts;

    frame_time m_last_frame_time;

};

}; // namespace ws
