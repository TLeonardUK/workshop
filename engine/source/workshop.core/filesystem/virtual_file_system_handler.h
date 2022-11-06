// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/filesystem/virtual_file_system_types.h"

#include <memory>
#include <string>
#include <vector>

namespace ws {

class stream;

// ================================================================================================
//  This class is the base class for protocol handlers that can be registered to the 
//  virtual file system.
// ================================================================================================
class virtual_file_system_handler
{
public:

    // Opens a stream to the given filename.
    virtual std::unique_ptr<stream> open(const char* path, bool for_writing) = 0;

    // Determines the type of the given filename.
    virtual virtual_file_system_path_type type(const char* path) = 0;

    // Removes a given file based on the filename.
    virtual bool remove(const char* path) = 0; 

    // Gets the time a file was modified.
    virtual bool modified_time(const char* path, virtual_file_system_time_point& timepoint) = 0;

    // Lists all the files or directories that exist in a given path.
    virtual std::vector<std::string> list(const char* path, virtual_file_system_path_type type) = 0;

};

}; // namespace workshop
