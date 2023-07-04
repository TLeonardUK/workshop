// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_imgui_manager.h"
#include "workshop.renderer/renderer.h"

#include "workshop.input_interface/input_interface.h"
#include "workshop.render_interface/ri_interface.h"

#include "workshop.renderer/systems/render_system_imgui.h"

#include "workshop.core/perf/profile.h"

#include "thirdparty/imgui/imgui.h"

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
        "Initialize ImGui Manager",
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
                const ImDrawVert& vert = cmd_list->VtxBuffer[vert_index];

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
                indices.emplace_back(vertex_offset + cmd_list->IdxBuffer[idx_index]);
            }

            // Append draw commands.
            for (size_t cmd_index = 0; cmd_index < cmd_list->CmdBuffer.Size; cmd_index++)
            {
                const ImDrawCmd& cmd = cmd_list->CmdBuffer[cmd_index];

                render_system_imgui::draw_command& render_cmd = commands.emplace_back();
                render_cmd.texture = reinterpret_cast<render_system_imgui::texture_id>(cmd.TextureId);
                render_cmd.offset = index_offset;
                render_cmd.count = cmd.ElemCount;
                render_cmd.display_size = vector2(data->DisplaySize.x, data->DisplaySize.y);
                render_cmd.display_pos = vector2(data->DisplayPos.x, data->DisplayPos.y);
                render_cmd.scissor = rect(
                    (cmd.ClipRect.x - render_cmd.display_pos.x) > 0 ? (cmd.ClipRect.x - render_cmd.display_pos.x) : 0.0f,
                    (cmd.ClipRect.y - render_cmd.display_pos.y) > 0 ? (cmd.ClipRect.y - render_cmd.display_pos.y) : 0.0f,
                    (cmd.ClipRect.z - cmd.ClipRect.x),
                    (cmd.ClipRect.w - cmd.ClipRect.y)
                );

                index_offset += cmd.ElemCount;
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
    auto iter = std::find_if(m_contexts.begin(), m_contexts.end(), [](auto& context) {
        return context->free;
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

    ImGui::SetCurrentContext(new_context->context);
//    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
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
        imgui->register_texture(std::move(texture), true);

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

    io.KeyShift = m_input.is_modifier_down(input_modifier::shift);
    io.KeyCtrl = m_input.is_modifier_down(input_modifier::ctrl);
    io.KeyAlt = m_input.is_modifier_down(input_modifier::alt);
    io.KeySuper = m_input.is_modifier_down(input_modifier::gui);

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

    m_input.set_mouse_capture(ImGui::IsAnyMouseDown());

    ImGui::NewFrame();
}

}; // namespace ws
