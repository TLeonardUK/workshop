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
//  Transaction thats created whenever the transform of a set of objects is changed
// ================================================================================================
class editor_transaction_change_object_transform
    : public editor_transaction
{
public:
    editor_transaction_change_object_transform(engine& engine, editor& editor);

    void add_object(object obj, 
        const vector3& previous_world_location, const quat& previous_world_rotation, const vector3& previous_world_scale,
        const vector3& new_world_location, const quat& new_world_rotation, const vector3& new_world_scale
    );

    virtual void execute() override;
    virtual void rollback() override;
    virtual std::string get_name() override;

private:
    editor& m_editor;
    engine& m_engine;

    struct object_state
    {
        object obj;
        vector3 world_location;
        quat world_rotation;
        vector3 world_scale;
    };

    std::vector<object_state> m_new;
    std::vector<object_state> m_previous;


};

}; // namespace ws
