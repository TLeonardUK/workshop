// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_imgui_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/systems/render_system_imgui.h"

#include "workshop.core/perf/profile.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include "workshop.input_interface/input_interface.h"
#include "workshop.render_interface/ri_interface.h"

#include "workshop.core/drawing/imgui.h"

namespace ws {

imgui_scope::imgui_scope(render_imgui_manager& manager, const char* name)
    : m_manager(manager)
{
    m_manager.m_scope_mutex.lock();
    m_manager.enter_scope(name);
}

imgui_scope::~imgui_scope()
{
    m_manager.leave_scope();
    m_manager.m_scope_mutex.unlock();
}

render_imgui_manager::render_imgui_manager(renderer& renderer, input_interface& input)
    : m_renderer(renderer)
    , m_input(input)
{
}

const char* render_imgui_manager::get_clipboard_text_callback(void* userdata)
{
    render_imgui_manager* manager = static_cast<render_imgui_manager*>(userdata);

    manager->m_clipboard_text = manager->m_input.get_clipboard_text();
    return manager->m_clipboard_text.data();
}

void render_imgui_manager::set_clipboard_text_callback(void* userdata, const char* text)
{
    render_imgui_manager* manager = static_cast<render_imgui_manager*>(userdata);
    manager->m_input.set_clipboard_text(text);
}

void render_imgui_manager::register_init(init_list& list)
{
    list.add_step(
        "ImGui Manager",
        [this, &list]() -> result<void> { return create_imgui(list); },
        [this]() -> result<void> { return destroy_imgui(); }
    );
}

void render_imgui_manager::step(frame_time& time)
{
    std::scoped_lock lock(m_scope_mutex);
    
    profile_marker(profile_colors::render, "Step ImGui");

    m_last_frame_time = time;

    std::vector<render_system_imgui::draw_command> commands;
    std::vector<render_system_imgui::vertex> vertices;
    std::vector<uint16_t> indices;

    // Drain render commands and reset contexts ready for the next frame.
    for (auto& ctx : m_contexts)
    {
        if (ctx->free)
        {
            continue;
        }

        ctx->free = true;

        ImGui::SetCurrentContext(ctx->context);
        ImGui::Render();

        ImDrawData* data = ImGui::GetDrawData();
        if (data->TotalVtxCount == 0)
        {
            continue;
        }
        
        vertices.reserve(vertices.size() + data->TotalVtxCount);
        indices.reserve(indices.size() + data->TotalIdxCount);
        commands.reserve(commands.size() + data->CmdListsCount);

        float color_scale = 1.0f / 255.0f;

        for (size_t cmd_list_index = 0; cmd_list_index < data->CmdListsCount; cmd_list_index++)
        {
            const ImDrawList* cmd_list = data->CmdLists[cmd_list_index];

            size_t vertex_offset = vertices.size();
            size_t index_offset = indices.size();

            // Emplace vertices
            for (size_t vert_index = 0; vert_index < cmd_list->VtxBuffer.Size; vert_index++)
            {
                const ImDrawVert& vert = cmd_list->VtxBuffer[static_cast<int>(vert_index)];

                render_system_imgui::vertex& render_vert = vertices.emplace_back();
                render_vert.position.x = vert.pos.x;
                render_vert.position.y = vert.pos.y;
                render_vert.uv.x = vert.uv.x;
                render_vert.uv.y = vert.uv.y;
                render_vert.color.x = ((vert.col >> IM_COL32_R_SHIFT) & 0xFF) * color_scale;
                render_vert.color.y = ((vert.col >> IM_COL32_G_SHIFT) & 0xFF) * color_scale;
                render_vert.color.z = ((vert.col >> IM_COL32_B_SHIFT) & 0xFF) * color_scale;
                render_vert.color.w = ((vert.col >> IM_COL32_A_SHIFT) & 0xFF) * color_scale;
            }

            // Emplace indices
            for (size_t idx_index = 0; idx_index < cmd_list->IdxBuffer.Size; idx_index++)
            {
                indices.emplace_back((uint16_t)(vertex_offset + cmd_list->IdxBuffer[static_cast<int>(idx_index)]));
            }

            // Append draw commands.
            for (size_t cmd_index = 0; cmd_index < cmd_list->CmdBuffer.Size; cmd_index++)
            {
                const ImDrawCmd& cmd = cmd_list->CmdBuffer[static_cast<int>(cmd_index)];

                render_system_imgui::draw_command& render_cmd = commands.emplace_back();
                render_cmd.texture = cmd.TextureId;
                render_cmd.offset = index_offset + cmd.IdxOffset;
                render_cmd.count = cmd.ElemCount;
                render_cmd.display_size = vector2(data->DisplaySize.x, data->DisplaySize.y);
                render_cmd.display_pos = vector2(data->DisplayPos.x, data->DisplayPos.y);
                render_cmd.scissor = rect(
                    (cmd.ClipRect.x - render_cmd.display_pos.x) > 0 ? (cmd.ClipRect.x - render_cmd.display_pos.x) : 0.0f,
                    (cmd.ClipRect.y - render_cmd.display_pos.y) > 0 ? (cmd.ClipRect.y - render_cmd.display_pos.y) : 0.0f,
                    (cmd.ClipRect.z - cmd.ClipRect.x),
                    (cmd.ClipRect.w - cmd.ClipRect.y)
                );
            }
        }
    }  

    m_renderer.get_command_queue().queue_command("Queue ImGui Draw", [renderer = &m_renderer, vertices, indices, commands]() mutable {

        render_system_imgui* imgui = renderer->get_system<render_system_imgui>();
        imgui->update_draw_data(commands, vertices, indices);

    });
}

bool render_imgui_manager::create_imgui(init_list& list)
{
    return true;
}

bool render_imgui_manager::destroy_imgui()
{
    m_renderer.drain();

    for (auto& context : m_contexts)
    {
        ImGui::DestroyContext(context->context);
    }

    m_contexts.clear();

    return true;
}

void render_imgui_manager::enter_scope(const char* name)
{
    render_imgui_manager::context* ctx = create_context(name);
    ImGui::SetCurrentContext(ctx->context);
}

void render_imgui_manager::leave_scope()
{
    // Nothing needed here right now.
}

render_imgui_manager::context* render_imgui_manager::create_context(const char* name)
{
    auto iter = std::find_if(m_contexts.begin(), m_contexts.end(), [&name](auto& context) {
        return context->free && context->name == name;
    });

    if (iter != m_contexts.end())
    {
        context* ctx = iter->get();
        ctx->free = false;

        ImGui::SetCurrentContext(ctx->context);
        start_context_frame(ctx);

        return ctx;
    }

    // Share font atlas between contexts.
    ImFontAtlas* atlas = nullptr;
    if (!m_contexts.empty())
    {
        ImGui::SetCurrentContext(m_contexts[0]->context);
        atlas = ImGui::GetIO().Fonts;
    }

    std::unique_ptr<context> new_context = std::make_unique<context>();
    new_context->free = false;
    new_context->context = ImGui::CreateContext(atlas);
    new_context->name = name;

    ImGui::SetCurrentContext(new_context->context);

    // Setup a reasonable style.
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.6000000238418579f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.FrameRounding = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.IndentSpacing = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 14.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.TabBorderSize = 0.0f;
    style.TabMinWidthForCloseButton = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5921568870544434f, 0.5921568870544434f, 0.5921568870544434f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.321568638086319f, 0.321568638086319f, 0.3333333432674408f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.3529411852359772f, 0.3529411852359772f, 0.3725490272045135f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.3529411852359772f, 0.3529411852359772f, 0.3725490272045135f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.321568638086319f, 0.321568638086319f, 0.3333333432674408f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.7f);

    // Setup input keymap.
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "save:imgui.ini";
    io.LogFilename = "save:imgui.log";
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.KeyMap[ImGuiKey_Tab] = (int)input_key::tab;
    io.KeyMap[ImGuiKey_LeftArrow] = (int)input_key::left;
    io.KeyMap[ImGuiKey_RightArrow] = (int)input_key::right;
    io.KeyMap[ImGuiKey_UpArrow] = (int)input_key::up;
    io.KeyMap[ImGuiKey_DownArrow] = (int)input_key::down;
    io.KeyMap[ImGuiKey_PageUp] = (int)input_key::page_up;
    io.KeyMap[ImGuiKey_PageDown] = (int)input_key::page_down;
    io.KeyMap[ImGuiKey_Home] = (int)input_key::home;
    io.KeyMap[ImGuiKey_End] = (int)input_key::end;
    io.KeyMap[ImGuiKey_Insert] = (int)input_key::insert;
    io.KeyMap[ImGuiKey_Delete] = (int)input_key::del;
    io.KeyMap[ImGuiKey_Backspace] = (int)input_key::backspace;
    io.KeyMap[ImGuiKey_Space] = (int)input_key::space;
    io.KeyMap[ImGuiKey_Enter] = (int)input_key::enter;
    io.KeyMap[ImGuiKey_Escape] = (int)input_key::escape;
    io.KeyMap[ImGuiKey_A] = (int)input_key::a;
    io.KeyMap[ImGuiKey_C] = (int)input_key::c;
    io.KeyMap[ImGuiKey_V] = (int)input_key::v;
    io.KeyMap[ImGuiKey_X] = (int)input_key::x;
    io.KeyMap[ImGuiKey_Y] = (int)input_key::y;
    io.KeyMap[ImGuiKey_Z] = (int)input_key::z;

    io.SetClipboardTextFn = &set_clipboard_text_callback;
    io.GetClipboardTextFn = &get_clipboard_text_callback;
    io.ClipboardUserData = this;

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    if (m_contexts.empty())
    {
        create_font_resources();
    }

    context* new_context_ptr = new_context.get();
    start_context_frame(new_context_ptr);

    m_contexts.push_back(std::move(new_context));

    return new_context_ptr;
}

void render_imgui_manager::free_context(context* context)
{
    context->free = true;
}

void render_imgui_manager::create_font_resources()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

    // Merge in the font-awesome font.
    float base_font_size = 11.0f;//13.0f;
    float icon_font_size = base_font_size;// * 2.0f / 3.0f;
    
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = icon_font_size;
    icons_config.GlyphOffset = ImVec2(1, 1);
    io.Fonts->AddFontFromFileTTF("data:fonts/core/" FONT_ICON_FILE_NAME_FAS, icon_font_size, &icons_config, icons_ranges);

    // Create the actual texture atlas.
    unsigned char* pixels;
    int texture_width;
    int texture_height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &texture_width, &texture_height);

    std::vector<uint8_t> data;
    data.resize(texture_width * texture_height * 4);
    memcpy(data.data(), pixels, data.size());

    m_renderer.get_command_queue().queue_command("Create ImGui Font", [renderer = &m_renderer, texture_width, texture_height, data]() mutable {

        render_system_imgui* imgui = renderer->get_system<render_system_imgui>();
        ri_interface& ri = renderer->get_render_interface();

        ri_texture::create_params params;
        params.width = texture_width;
        params.height = texture_height;
        params.dimensions = ri_texture_dimension::texture_2d;
        params.format = ri_texture_format::R8G8B8A8;
        params.data = data;

        std::unique_ptr<ri_texture> texture = ri.create_texture(params, "ImGui Font");
        imgui->set_default_texture(std::move(texture));

    });
}

void render_imgui_manager::start_context_frame(context* context)
{
    ImGuiIO& io = ImGui::GetIO();
    
    // Display attributes
    io.DisplaySize = ImVec2(static_cast<float>(m_renderer.get_display_width()), static_cast<float>(m_renderer.get_display_height()));
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    io.DeltaTime = m_last_frame_time.delta_seconds;

    // Input state.
    vector2 mouse_pos = m_input.get_mouse_position();
    io.MousePos = ImVec2(mouse_pos.x, mouse_pos.y);

    for (int i = 0; i < sizeof(io.MouseDown) / sizeof(*io.MouseDown); i++)
    {
        io.MouseDown[i] = m_input.is_key_down(static_cast<input_key>(static_cast<int>(input_key::mouse_0) + i));
    }

    io.MouseWheel = (float)m_input.get_mouse_wheel_delta(false);
    io.MouseWheelH = (float)m_input.get_mouse_wheel_delta(true);

    std::string input = m_input.get_input();
    io.AddInputCharactersUTF8(input.c_str());

    for (size_t i = 0; i < static_cast<size_t>(input_key::count); i++)
    {
        input_key k = static_cast<input_key>(i);
        io.KeysDown[i] = m_input.is_key_down(k);
    }

    io.KeyShift = m_input.is_key_down(input_key::shift);
    io.KeyCtrl = m_input.is_key_down(input_key::ctrl);
    io.KeyAlt = m_input.is_key_down(input_key::alt);
    io.KeySuper = m_input.is_key_down(input_key::gui);

    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0)
    {
        switch (ImGui::GetMouseCursor())
        {
        case ImGuiMouseCursor_None:			m_input.set_mouse_cursor(input_cursor::none);		break;
        case ImGuiMouseCursor_Arrow:		m_input.set_mouse_cursor(input_cursor::arrow);	    break;
        case ImGuiMouseCursor_TextInput:	m_input.set_mouse_cursor(input_cursor::ibeam);	    break;
        case ImGuiMouseCursor_ResizeAll:	m_input.set_mouse_cursor(input_cursor::size_all);	break;
        case ImGuiMouseCursor_ResizeNS:		m_input.set_mouse_cursor(input_cursor::size_ns);	break;
        case ImGuiMouseCursor_ResizeEW:		m_input.set_mouse_cursor(input_cursor::size_we);	break;
        case ImGuiMouseCursor_ResizeNESW:	m_input.set_mouse_cursor(input_cursor::size_nesw);	break;
        case ImGuiMouseCursor_ResizeNWSE:	m_input.set_mouse_cursor(input_cursor::size_nwse);	break;
        }
    }

    if (io.WantSetMousePos)
    {
        m_input.set_mouse_position(vector2(io.MousePos.x, io.MousePos.y));
    }

    ImGui::NewFrame();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();
}

}; // namespace ws
