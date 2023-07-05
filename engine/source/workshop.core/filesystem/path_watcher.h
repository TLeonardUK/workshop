// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/debug/debug.h"

#include <filesystem>

namespace ws {

// ================================================================================================
//  Watches for changes to a path on the filesystem.
// ================================================================================================
class path_watcher
{
public:

	enum class event_type
	{
		modified
	};

	struct event
	{
		event_type type;
		std::filesystem::path path;
	};

	virtual ~path_watcher() {};

	// Returns the next change detected, or returns false if none seen.
	virtual bool get_next_change(event& out_event) = 0;

};

// ================================================================================================
//  Creates a path watcher 
// ================================================================================================
std::unique_ptr<path_watcher> watch_path(const std::filesystem::path& path);

}; // namespace workshop
