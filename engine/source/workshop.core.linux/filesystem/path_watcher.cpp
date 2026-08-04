// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/path_watcher.h"
#include "workshop.core/containers/string.h"

#include <array>

namespace ws {

// linux-todo

class linux_path_watcher : public path_watcher
{
public:
    linux_path_watcher();
    virtual ~linux_path_watcher();

    bool init(const std::filesystem::path& path);
    void poll_changes();

    virtual bool get_next_change(event& out_event) override;

private:

};

linux_path_watcher::linux_path_watcher()
{
}

bool linux_path_watcher::init(const std::filesystem::path& path)
{
    return false;
}

linux_path_watcher::~linux_path_watcher()
{
}

void linux_path_watcher::poll_changes()
{

}

bool linux_path_watcher::get_next_change(event& out_event)
{
    poll_changes();
    return false;
}

std::unique_ptr<path_watcher> watch_path(const std::filesystem::path& path)
{
    std::unique_ptr<linux_path_watcher> result = std::make_unique<linux_path_watcher>();
    if (!result->init(path))
    {
        return nullptr;
    }

    return result;
}

}; // namespace workshop
