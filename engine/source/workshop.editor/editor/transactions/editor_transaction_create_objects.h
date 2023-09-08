// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_undo_stack.h"
#include "workshop.editor/editor/editor.h"
#include "workshop.engine/ecs/object.h"

namespace ws {

class engine;

// ================================================================================================
//  Transaction thats created whenever an object is created.
// ================================================================================================
class editor_transaction_create_objects
    : public editor_transaction
{
public:
    editor_transaction_create_objects(engine& engine, editor& editor, const std::vector<object>& handles);

    virtual void execute() override;
    virtual void rollback() override;
    virtual std::string get_name() override;

private:
    struct state
    {
        object handle = null_object;
        std::vector<uint8_t> serialized;
        bool alive = false;
    };

    editor& m_editor;
    engine& m_engine;

    std::vector<state> m_states;

};

}; // namespace ws
