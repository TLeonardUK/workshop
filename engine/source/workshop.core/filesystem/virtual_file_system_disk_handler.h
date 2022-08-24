// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/filesystem/virtual_file_system_handler.h"

namespace ws {

// ================================================================================================
//  This class is the base class for protocol handlers that can be registered to the 
//  virtual file system.
// ================================================================================================
class virtual_file_system_disk_handler : public virtual_file_system_handler
{
public:

    virtual_file_system_disk_handler(const std::string& root, bool read_only);

    virtual std::unique_ptr<stream> open(const char* path, bool for_writing) override;
    virtual virtual_file_system_path_type type(const char* path) override;
    virtual std::vector<std::string> list(const char* path, virtual_file_system_path_type type) override;

private:
    std::string m_root;
    bool m_read_only;

};

}; // namespace workshop
