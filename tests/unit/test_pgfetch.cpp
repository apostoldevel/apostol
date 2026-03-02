#if defined(WITH_POSTGRESQL) && defined(WITH_CURL)

#include <catch2/catch_test_macros.hpp>

#include "PGFetch/PGFetch.hpp"
#include "apostol/application.hpp"
#include "apostol/curl_client.hpp"
#include "apostol/event_loop.hpp"
#include "apostol/http.hpp"
#include "apostol/pg.hpp"

using namespace apostol;

// ─── PGFetch unit tests (no live PG/HTTP needed) ────────────────────────────

namespace
{

struct PGFetchTestFixture
{
    Application app{"pgfetch_test"};
    EventLoop loop;

    PGFetchTestFixture()
    {
        app.setup_db(loop, "host=localhost dbname=test", 0, 0);
    }
};

} // anonymous namespace

TEST_CASE("PGFetch — construct and destruct", "[pgfetch]")
{
    PGFetchTestFixture f;
    PGFetch mod(f.app, f.loop);
    CHECK(mod.enabled());
    CHECK(mod.name() == "PGFetch");
}

TEST_CASE("PGFetch — enabled by default", "[pgfetch]")
{
    PGFetchTestFixture f;
    PGFetch mod(f.app, f.loop);
    CHECK(mod.enabled());
}

TEST_CASE("PGFetch — execute always returns false", "[pgfetch]")
{
    PGFetchTestFixture f;
    PGFetch mod(f.app, f.loop);

    HttpRequest req;
    req.method = "GET";
    req.path = "/anything";
    HttpResponse resp;

    // PGFetch is a helper — never handles HTTP requests
    CHECK_FALSE(mod.execute(req, resp));
}

TEST_CASE("PGFetch — heartbeat without crash", "[pgfetch]")
{
    PGFetchTestFixture f;
    PGFetch mod(f.app, f.loop);

    // Heartbeat with empty queue should be a no-op
    mod.heartbeat(std::chrono::system_clock::now());
}

#endif // WITH_POSTGRESQL && WITH_CURL
