// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/log_handler.h"

#include <functional>

namespace ws {

// ================================================================================================
//  Represents a tree of assets displayed to the end user. 
//  Allows us to perform simple caching without recalculating all this information continually.
// ================================================================================================
class asset_tree 
{
public:
    struct file
    {
        std::string name;
        std::string path;
    };

    struct dir
    {
        dir* parent = nullptr;
        std::vector<std::unique_ptr<dir>> children;

        std::vector<std::unique_ptr<file>> files;

        std::string name;
        std::string path;

        bool retrieved_children = false;
    };

public:

    asset_tree();

    // Forces a refresh of the state of a directory.
    void refresh(const dir& dir);

    // Returns true if the given directory is out of date and needs refreshing.
    bool is_out_of_date(const dir& dir);

    // Gets the root of the tree.
    const dir& get_root();

private:
    void add_node(dir& parent, const char* path, const std::vector<std::string>& fragments);

private:
    dir m_root;

};

}; // namespace ws
