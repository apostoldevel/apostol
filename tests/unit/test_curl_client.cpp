#ifdef WITH_CURL

#include <catch2/catch_test_macros.hpp>

#include "apostol/curl_client.hpp"
#include "apostol/event_loop.hpp"

using namespace apostol;

// ─── CurlClient construction ───────────────────────────────────────────────

TEST_CASE("CurlClient — construct and destruct", "[curl_client]")
{
    EventLoop loop;
    CurlClient curl(loop);
    // Smoke test: no crash
}

TEST_CASE("CurlClient — set_timeout and set_proxy", "[curl_client]")
{
    EventLoop loop;
    CurlClient curl(loop);
    curl.set_timeout(5000);
    curl.set_proxy("socks5://127.0.0.1:9050");
    // Smoke test: no crash
}

// ─── Live HTTP tests (require network access) ──────────────────────────────
//
// These tests perform real HTTP requests. They are tagged [.live] so they
// don't run by default. Run them explicitly:
//   ctest --test-dir cmake-build-debug -R "CurlClient.*live" -V
//

TEST_CASE("CurlClient — GET httpbin.org/get", "[curl_client][.live]")
{
    EventLoop loop;
    CurlClient curl(loop);
    curl.set_timeout(10000);

    bool done = false;
    int  status = 0;

    curl.get("https://httpbin.org/get", {},
        [&](CurlResponse resp) {
            status = resp.status_code;
            done = true;
            loop.stop();
        },
        [&](int code, std::string_view err) {
            FAIL("curl failed: " << err);
            loop.stop();
        });

    // Add a safety timeout to avoid hanging
    loop.add_timer(std::chrono::seconds(15), [&]() { loop.stop(); }, false);

    loop.run();

    CHECK(done);
    CHECK(status == 200);
}

TEST_CASE("CurlClient — POST httpbin.org/post", "[curl_client][.live]")
{
    EventLoop loop;
    CurlClient curl(loop);
    curl.set_timeout(10000);

    bool done = false;

    curl.post("https://httpbin.org/post", R"({"test":true})",
        {{"Content-Type", "application/json"}},
        [&](CurlResponse resp) {
            CHECK(resp.status_code == 200);
            done = true;
            loop.stop();
        },
        [&](int code, std::string_view err) {
            FAIL("curl failed: " << err);
            loop.stop();
        });

    loop.add_timer(std::chrono::seconds(15), [&]() { loop.stop(); }, false);
    loop.run();

    CHECK(done);
}

#endif // WITH_CURL
