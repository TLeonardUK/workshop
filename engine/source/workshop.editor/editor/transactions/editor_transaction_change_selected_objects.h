// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_undo_stack.h"
#include "workshop.editor/editor/editor.h"
#include "workshop.engine/ecs/object.h"

namespace ws {

// ================================================================================================
//  Transaction thats created whenever the selected object in the editor is changed.
// ================================================================================================
class editor_transaction_change_selected_objects 
    : public editor_transaction
{
public:
    editor_transaction_change_selected_objects(editor& editor, const std::vector<object>& new_objects);

    virtual void execute() override;
    virtual void rollback() override;
    virtual std::string get_name() override;

private:
    editor& m_editor;
    std::vector<object> m_new;
    std::vector<object> m_previous;

};

}; // namespace ws
