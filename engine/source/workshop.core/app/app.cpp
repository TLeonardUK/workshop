// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/app/app.h"
#include "workshop.core/utils/init_list.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/debug/debug.h"

namespace ws {

app::app()
{
    db_assert(s_instance == nullptr);
    s_instance = this;
}

app::~app()
{
    db_assert(s_instance == this);
    s_instance = nullptr;
}

app& app::instance()
{
    db_assert(s_instance != nullptr);
    return *s_instance;   
}

result<void> app::run()
{
    init_list list;

    // Register initialization steps for derived classes.
    register_init(list);

    // This step always comes last.
    list.add_step(
        "App",
        [this]() -> result<void> { return start(); },
        [this]() -> result<void> { return stop(); }
    );

    if (result<void> ret = list.init(); !ret)
    {
        return ret;
    }

    if (result<void> ret = loop(); !ret)
    {
        return ret;
    }

    return true;
}

void app::register_init(init_list& list)
{
    list.add_step(
        "Debug Symbols",
        [this]() -> result<void> { return db_load_symbols(); },
        [this]() -> result<void> { return db_unload_symbols(); }
    );
}

result<void> app::start()
{
    return true;
}

result<void> app::loop()
{
    // If no implementation is given we just finish immediately.
    return true;
};

result<void> app::stop()
{
    return true;
}

void app::quit()
{
    m_quit_requested = true;
}

bool app::is_quitting()
{
    return m_quit_requested;
}

}; // namespace ws
