/// benchmark/apostol.v2/WebServer.cpp

#include "WebServer.hpp"
#include "apostol/application.hpp"
#include "apostol/http.hpp"

#ifdef WITH_POSTGRESQL
#include "apostol/pg.hpp"
#include "apostol/pg_exec.hpp"
#endif

#include <ctime>
#include <fmt/format.h>

namespace apostol
{

static constexpr std::string_view k_json_ct = "application/json; charset=utf-8";

WebServer::WebServer(Application& app)
#ifdef WITH_POSTGRESQL
    : pool_(app.has_db_pool() ? &app.db_pool() : nullptr)
#endif
{
    routes_.set_info("Apostol v2 Benchmark", "5.0.0",
                     "Minimal benchmark endpoints for performance testing");
}

void WebServer::init_routes()
{
    routes_.add_route("GET", "/api/v5/ping",
        [](const HttpRequest&, HttpResponse& resp, const PathParams&) {
            resp.set_status(HttpStatus::ok)
                .set_body(R"({"ok":true,"message":"OK"})", std::string(k_json_ct));
        })
        .summary("Ping healthcheck")
        .tag("Benchmark")
        .response(200, "OK");

    routes_.add_route("GET", "/api/v5/time",
        [](const HttpRequest&, HttpResponse& resp, const PathParams&) {
            resp.set_status(HttpStatus::ok)
                .set_body(fmt::format("{{\"serverTime\":{}}}", std::time(nullptr)),
                          std::string(k_json_ct));
        })
        .summary("Server time")
        .tag("Benchmark")
        .response(200, "OK");

#ifdef WITH_POSTGRESQL
    if (pool_) {
        routes_.add_route("GET", "/api/v5/db/ping",
            [this](const HttpRequest& req, HttpResponse& resp, const PathParams&) {
                exec_sql(*pool_, req, resp, "SELECT 1",
                    [](std::shared_ptr<HttpConnection> conn,
                       std::vector<PgResult> results) {
                        HttpResponse r;
                        if (!results.empty() && results[0].ok()) {
                            r.set_status(HttpStatus::ok)
                                .set_body(R"({"ok":true,"message":"OK"})",
                                          "application/json; charset=utf-8");
                        } else {
                            reply_error(r, HttpStatus::internal_server_error,
                                        "query failed");
                        }
                        conn->send_response(r);
                    });
            })
            .summary("DB ping (SELECT 1)")
            .tag("Benchmark")
            .response(200, "OK")
            .response(500, "Database error");
    }
#endif
}

} // namespace apostol
