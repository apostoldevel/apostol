/// benchmark/apostol.v2/WebServer.hpp
///
/// Benchmark WebServer module for apostol.v2 (RoutedModule edition).
/// Serves static JSON endpoints via RouteManager — measures real RoutedModule overhead.
///
///   GET /api/v5/ping     → {"ok":true,"message":"OK"}
///   GET /api/v5/time     → {"serverTime":<unix_epoch>}
///   GET /api/v5/db/ping  → {"ok":true,"message":"OK"}  (SELECT 1 via PgPool)
///   GET /docs            → Swagger UI (auto from RoutedModule)

#pragma once

#include "apostol/routed_module.hpp"

#include <string_view>

namespace apostol
{

class Application;
class PgPool;

class WebServer final : public RoutedModule
{
public:
    explicit WebServer(Application& app);

    std::string_view name()    const override { return "WebServer"; }
    bool             enabled() const override { return true; }

protected:
    void init_routes() override;

private:
#ifdef WITH_POSTGRESQL
    PgPool* pool_{nullptr};
#endif
};

} // namespace apostol
