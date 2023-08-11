// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/utils/init_list.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/utils/singleton.h"
#include "workshop.engine/engine/engine.h"

namespace ws {

// Describes what parts of the editor UI should be shown.
enum class editor_mode
{
    // Editor is fully open.
    editor,

    // Editor UI is hidden and the game is shown.
    game,
};

// ================================================================================================
//  This is the core editor class, its responsible for owning all the individual components 
//  required to render the editor ui.
// ================================================================================================
class editor 
    : public singleton<editor>
{
public:

    editor(engine& in_engine);
    ~editor();

    // Registers all the steps required to initialize the engine.
    // Interacting with this class without successfully running these steps is undefined.
    void register_init(init_list& list);

    // Advances the state of the engine. This should be called repeatedly in the main loop of the 
    // application. 
    // 
    // Calling this does not guarantee a new render frame, or the state of the simulation being advanced, 
    // as the engine runs entirely asyncronously. This function mearly provides the opportunity for the engine
    // to advance if its in a state that permits it.
    void step(const frame_time& time);

    // Switches to the given editor mode.
    void set_editor_mode(editor_mode mode);

protected:

    void draw_dockspace();
    void draw_main_menu();

protected:

    engine& m_engine;

    editor_mode m_editor_mode = editor_mode::game;

};

}; // namespace ws
