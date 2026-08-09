// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/path_watcher.h"

namespace ws {

// ================================================================================================
//  Stub implementation of a path watcher. The stub platform has no filesystem notification
//  apis available, so no changes are ever reported.
// ================================================================================================
class stub_path_watcher : public path_watcher
{
public:
    virtual bool get_next_change(event& out_event) override;

};

bool stub_path_watcher::get_next_change(event& out_event)
{
    return false;
}

std::unique_ptr<path_watcher> watch_path(const std::filesystem::path& path)
{
    return std::make_unique<stub_path_watcher>();
}

}; // namespace ws
