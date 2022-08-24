// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"

namespace ws {

struct init_list;

// ================================================================================================
//  This is the base class for all applications. A single instance is create during startup
//  and exists for the lifetime of the process. It controls the basic process lifecycle.
// ================================================================================================
class app
{
public:

    app();
    virtual ~app();

    // Gets the running instance of the application.
    static app& instance();

    // Called by whatever code creates the application to
    // run the application through its full process lifecycle. 
    // This will block until the application finishes, the result can be used
    // as the exit code.
    result<void> run();
    
    // Starts the process of closing down the application.
    void quit();

    // Gets a simple name for the application. 
    // This should match the folder name the applications assets/etc are contained in.
    // Should be alphanumeric without path-forbidden chars as its used in filenames.
    virtual std::string get_name() = 0;

protected:

    // Called when application is initializing, you can use this to 
    // register any initialization steps. You should always call the base
    // version of this function first before registering your own steps to
    // maintain ordering.
    virtual void register_init(init_list& list);

    // Called to start the application. 
    // Returning failure will immediately abort.
    virtual result<void> start();

    // Runs the main loop of the application. 
    // The application is considered to be shutting down when this returns.
    // If the application has nothing to do in the loop it should just call wait_for_close_request.
    virtual result<void> loop();

    // Called to teardown the application. 
    // Called when run finishes.
    virtual result<void> stop();

    // Returns true if something has requested the application to close.
    bool is_quitting();
    
private:

    inline static app* s_instance = nullptr;

    bool m_quit_requested = false;

};

}; // namespace ws
