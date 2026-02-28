#pragma once

// ─── Helpers.hpp ─────────────────────────────────────────────────────────────
//
// Registration header for all Helper modules (persistent background tasks).
//
// create_helpers() is called from Application::on_helper_start() in main.cpp.

#include "apostol/application.hpp"

#ifdef WITH_POSTGRESQL
#include "PGFetch/PGFetch.hpp"
#endif

namespace apostol
{

/// Instantiate and register all Helper modules with the module manager.
static inline void create_helpers(Application& app, EventLoop& loop)
{
#ifdef WITH_POSTGRESQL
    if (app.module_enabled("PGFetch") && app.has_db_pool()) {
        app.module_manager().add_module(
            std::make_unique<PGFetch>(app, loop));
    }
#else
    (void)app;
    (void)loop;
#endif
}

} // namespace apostol
