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
//  Transaction thats created whenever a component is created.
// ================================================================================================
class editor_transaction_create_components
    : public editor_transaction
{
public:
    editor_transaction_create_components(engine& engine, editor& editor, object handle, const std::vector<std::type_index>& component_types);

    virtual void execute() override;
    virtual void rollback() override;
    virtual std::string get_name() override;

private:
    struct component_info
    {
        std::type_index type = typeid(void);
        std::vector<uint8_t> serialized;
    };

    editor& m_editor;
    engine& m_engine;
    object m_handle = null_object;
    bool m_alive = false;

    std::vector<component_info> m_components;

};

}; // namespace ws
