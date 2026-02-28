#pragma once

// ─── Workers.hpp ─────────────────────────────────────────────────────────────
//
// Registration header for all Worker modules (HTTP/WebSocket request handlers).
//
// create_workers() is called from Application::on_worker_start() in main.cpp.

#include "apostol/application.hpp"

#ifdef WITH_POSTGRESQL
#include "PGHTTP/PGHTTP.hpp"
#endif

#include "WebServer/WebServer.hpp"

namespace apostol
{

/// Instantiate and register all Worker modules with the module manager.
static inline void create_workers(Application& app)
{
#ifdef WITH_POSTGRESQL
    if (app.module_enabled("PGHTTP") && app.has_db_pool())
        app.module_manager().add_module(std::make_unique<PGHTTP>(app));
#endif

    // WebServer — last in chain (fallback to static files)
    app.module_manager().add_module(std::make_unique<WebServer>(app));
}

} // namespace apostol
