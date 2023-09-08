// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace ws {

// ================================================================================================
//  Base class for all transactions that can be performed and reversed.
// ================================================================================================
class editor_transaction
{
public:

    // Runs the transaction.
    virtual void execute() = 0;

    // Rollsback whatever changes a previous execution caused.
    virtual void rollback() = 0;

    // Gets a name that describes the transaction and is used to reference this in the UI.
    virtual std::string get_name() = 0;

};

// ================================================================================================
//  Handles rolling trasactions forwards and backwards to support undo/redo in the editor.
// ================================================================================================
class editor_undo_stack
{
public:

    // Push a new operation onto the stack and executes it.
    void push(std::unique_ptr<editor_transaction> transaction);

    // Pops the next undo operation off the stack and performs it.
    void undo();

    // Pops the next redo operation off the stack and performs it.
    void redo();

    // Clears both the undo and redo stacks.
    void clear();

    // Gets the name of the next undo operation, or an empty
    // string if no more undo operations are available.
    std::string get_next_undo_name();

    // Gets the name of the next redo operation, or an empty
    // string if no more redo operations are available.
    std::string get_next_redo_name();

protected:
    static inline constexpr size_t k_max_stack_size = 100;

    std::vector<std::unique_ptr<editor_transaction>> m_undo_stack;
    std::vector<std::unique_ptr<editor_transaction>> m_redo_stack;


};

}; // namespace ws
