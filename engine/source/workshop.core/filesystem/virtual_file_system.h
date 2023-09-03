// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/singleton.h"

#include "workshop.core/filesystem/virtual_file_system_types.h"
#include "workshop.core/filesystem/virtual_file_system_handler.h"

#include <string>
#include <vector>
#include <mutex>

namespace ws {

class stream;

// ================================================================================================
//  This class implements a virtual file system. 
//  It essentially emulates a file-system on disk but with additional functionality.
//  
//  Filenames are always in the format:
//      protocol:folder/data.dat
// 
//  When a request to access a files data occurs, the virtual file system will look through the 
//  list of "handlers" that have been registered to it for one that handles the protocol part
//  of the filename, and then asks it for the data.
// 
//  This handler system allows us to access data is vastly different ways based on the protocol.
//  At the simplest level you could have a handler for say:
//      data:
//  Which would load from a folder on disk. And you could have another handler for say:
//      http:
//  Which would load from a network resource.
//  However the real power comes when you start adding domain-specific protocols such as:
//      savedata:
//  Which could be redirected to platform-specific save-data api rather than reading/writing to disk.
// 
//  On top of this its possible to register multiple handlers for the same protocol, each with 
//  different priorities. When file access is requested the file system will go through each
//  handler for the protocol in order of priority until it gets a success. This allows doing things
//  like having "overlays" onto the same path by say mods, or engine/game data.
// 
//  The file system is thread-safe.
// ================================================================================================
class virtual_file_system 
    : public singleton<virtual_file_system>
{
public:

    using handler_id = size_t;

    virtual_file_system();
    ~virtual_file_system();

    // Registers a new file system for handling a given file protocol. 
    // An id to uniquely identify this handler is returned, this can be later used to unregister it.
    handler_id register_handler(const char* protocol, int32_t priority, std::unique_ptr<virtual_file_system_handler>&& handler);

    // Unregisters a previously registered handler.
    // Ensure the handler is not being used.
    void unregister_handler(handler_id id);

    // Opens a stream to the given filename.
    std::unique_ptr<stream> open(const char* path, bool forWriting);

    // Determines the type of the given filename.
    virtual_file_system_path_type type(const char* path);

    // Determines if the given path exists.
    bool exists(const char* path);

    // Creates the given directory recursively.
    bool create_directory(const char* path);

    // Removes a given file from the file system, only valid if path is writable.
    bool remove(const char* path);

    // Renames the given filename.
    bool rename(const char* source, const char* destination);

    // Copies the given file from one path to another.
    bool copy(const char* source, const char* destination);

    // Gets the time a file was modified.
    bool modified_time(const char* path, virtual_file_system_time_point& timepoint);

    // Lists all the files or directories that exist in a given path.
    std::vector<std::string> list(const char* path, virtual_file_system_path_type type, bool filename_only = false, bool recursive = true);

    // Watches a path within the file system for modifications and raises events when they occur.
    std::unique_ptr<virtual_file_system_watcher> watch(const char* path, virtual_file_system_watcher::callback_t callback);

    // Invokes any pending callbacks for paths that are being watched via watch(). 
    void raise_watch_events();

    // Attempts to get the path on the host filesystem that the given vfs path will point towards.
    // If multiple handlers are registered for the path, the highest priority one will be used.
    // If no path can be calculated an empty string will be returned.
    std::string get_disk_location(const char* path);

public:

    // Normalizes a path.
    static std::string normalize(const char* path);

    // Cracks a path into its consituent parts.
    static void crack(const char* path, std::string& protocol, std::string& filename);

    // Swaps the protocol attached to the given path.
    static std::string replace_protocol(const char* path, const char* new_protocol);

    // Gets the parent directory of the given path.
    static std::string get_parent(const char* path);

    // Gets the extension on the given path.
    static std::string get_extension(const char* path);

    // Gets a pointer to all the handlers for the given protocol.
    std::vector<virtual_file_system_handler*> get_handlers(const std::string& protocol);

private:

    struct registered_handle
    {
        registered_handle()
            : id(0)
            , protocol("")
            , priority(0)
            , handler(nullptr)
        {
        }

        registered_handle(registered_handle&& handle)
            : id(handle.id)
            , protocol(handle.protocol)
            , priority(handle.priority)
            , handler(std::move(handle.handler))
        {
        }

        registered_handle& operator=(registered_handle&& other)
        {
            id = other.id;
            protocol = other.protocol;
            priority = other.priority;
            handler = std::move(other.handler);
            return *this;
        }

        handler_id id;
        std::string protocol;
        int32_t priority;
        std::unique_ptr<virtual_file_system_handler> handler;
    };

    std::recursive_mutex m_handlers_mutex;
    std::vector<registered_handle> m_handlers;

    size_t m_next_handle_id = 0;

};

}; // namespace workshop
