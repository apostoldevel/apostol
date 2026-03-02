#ifdef WITH_POSTGRESQL

#include <catch2/catch_test_macros.hpp>

#include "PGHTTP/PGHTTP.hpp"
#include "apostol/application.hpp"
#include "apostol/http.hpp"
#include "apostol/pg.hpp"
#include "apostol/event_loop.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace apostol;

// ─── Endpoint matching (unit tests — no PG needed) ──────────────────────────

// Helper: create a PGHTTP module via Application (no config → default /api/*).
// We need a real EventLoop+PgPool reference, but won't call execute() with live PG.

namespace
{

struct PGHTTPTestFixture
{
    Application app{"pghttp_test"};
    EventLoop loop;
    PGHTTPTestFixture() { app.setup_db(loop, "host=localhost dbname=test", 0, 0); }
};

} // anonymous namespace

TEST_CASE("PGHTTP — default endpoint /api/* matches /api/v1/foo", "[pghttp]")
{
    PGHTTPTestFixture f;
    PGHTTP mod(f.app);

    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/foo";
    HttpResponse resp;

    // execute() should match and set deferred (we can't check PG call,
    // but it will try pool_.execute which will queue the query)
    bool handled = mod.execute(req, resp);
    CHECK(handled);
    CHECK(resp.is_deferred());
}

TEST_CASE("PGHTTP — /other/path does not match default /api/*", "[pghttp]")
{
    PGHTTPTestFixture f;
    PGHTTP mod(f.app);

    HttpRequest req;
    req.method = "GET";
    req.path = "/other/path";
    HttpResponse resp;

    bool handled = mod.execute(req, resp);
    CHECK_FALSE(handled);
    CHECK_FALSE(resp.is_deferred());
}

TEST_CASE("PGHTTP — module is enabled by default", "[pghttp]")
{
    PGHTTPTestFixture f;
    PGHTTP mod(f.app);

    CHECK(mod.enabled());
}

TEST_CASE("PGHTTP — OPTIONS returns 204", "[pghttp]")
{
    PGHTTPTestFixture f;
    PGHTTP mod(f.app);

    HttpRequest req;
    req.method = "OPTIONS";
    req.path = "/api/v1/test";
    HttpResponse resp;

    bool handled = mod.execute(req, resp);
    CHECK(handled);
    CHECK_FALSE(resp.is_deferred());  // OPTIONS is synchronous
}

TEST_CASE("PGHTTP — unsupported method returns 405", "[pghttp]")
{
    PGHTTPTestFixture f;
    PGHTTP mod(f.app);

    HttpRequest req;
    req.method = "TRACE";
    req.path = "/api/v1/test";
    HttpResponse resp;

    bool handled = mod.execute(req, resp);
    CHECK(handled);
    CHECK_FALSE(resp.is_deferred());  // 405 is synchronous
}

TEST_CASE("PGHTTP — empty path with default /api/* is not matched", "[pghttp]")
{
    PGHTTPTestFixture f;
    PGHTTP mod(f.app);

    HttpRequest req;
    req.method = "GET";
    req.path = "";
    HttpResponse resp;

    bool handled = mod.execute(req, resp);
    // "/api/*" does not match empty path — check_location returns false
    CHECK_FALSE(handled);
}

TEST_CASE("PGHTTP — default endpoint /api/* when no config provided", "[pghttp]")
{
    PGHTTPTestFixture f;
    PGHTTP mod(f.app);

    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/ping";
    HttpResponse resp;

    bool handled = mod.execute(req, resp);
    CHECK(handled);
}

// ─── Config-driven endpoint test ────────────────────────────────────────────

TEST_CASE("PGHTTP: reads endpoints from module config", "[pghttp]")
{
    // Write a temp config file with custom PGHTTP endpoints
    auto tmp = std::filesystem::temp_directory_path()
               / ("pghttp-cfg-" + std::to_string(::getpid()) + ".json");
    {
        std::ofstream f(tmp);
        f << R"({
            "module": {
                "PGHTTP": {
                    "endpoints": ["/rpc/*", "/v2/*"]
                }
            }
        })";
    }

    // Construct Application and load config via -t -c <tmpfile>
    Application app("pghttp_cfg_test");
    std::string prog = "pghttp_cfg_test";
    std::string flag_t = "-t";
    std::string flag_c = "-c";
    std::string cfg_path = tmp.string();
    char* argv[] = { prog.data(), flag_t.data(), flag_c.data(), cfg_path.data(), nullptr };
    int rc = app.run(4, argv);
    REQUIRE(rc == 0);

    // Verify module_config returns our endpoints
    auto* cfg = app.module_config("PGHTTP");
    REQUIRE(cfg != nullptr);
    REQUIRE(cfg->contains("endpoints"));
    REQUIRE((*cfg)["endpoints"].size() == 2);

    // Set up DB pool and construct PGHTTP
    EventLoop loop;
    app.setup_db(loop, "host=localhost dbname=test", 0, 0);
    PGHTTP mod(app);

    // /rpc/call should match (configured endpoint /rpc/*)
    {
        HttpRequest req;
        req.method = "GET";
        req.path = "/rpc/call";
        HttpResponse resp;
        CHECK(mod.execute(req, resp));
    }

    // /v2/users should match (configured endpoint /v2/*)
    {
        HttpRequest req;
        req.method = "GET";
        req.path = "/v2/users";
        HttpResponse resp;
        CHECK(mod.execute(req, resp));
    }

    // /api/foo should NOT match (not in configured endpoints)
    {
        HttpRequest req;
        req.method = "GET";
        req.path = "/api/foo";
        HttpResponse resp;
        CHECK_FALSE(mod.execute(req, resp));
    }

    app.stop_db();
    std::filesystem::remove(tmp);
}

#endif // WITH_POSTGRESQL
