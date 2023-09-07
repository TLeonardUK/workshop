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
//  Transaction thats created whenever an object is deleted.
// ================================================================================================
class editor_transaction_delete_object
    : public editor_transaction
{
public:
    editor_transaction_delete_object(engine& engine, editor& editor, object handle);

    virtual void execute() override;
    virtual void rollback() override;
    virtual std::string get_name() override;

private:
    editor& m_editor;
    engine& m_engine;
    object m_handle = null_object;

    std::vector<uint8_t> m_serialized;

};

}; // namespace ws
