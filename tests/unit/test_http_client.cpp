#include <catch2/catch_test_macros.hpp>

#include "apostol/http_client.hpp"
#include "apostol/http.hpp"
#include "apostol/tcp.hpp"
#include "apostol/event_loop.hpp"

#include <chrono>
#include <fmt/format.h>
#include <memory>
#include <string>
#include <vector>

using namespace apostol;
using namespace std::chrono_literals;

// ─── Helpers ─────────────────────────────────────────────────────────────────

// Minimal HTTP server: accepts connections, parses request, returns canned response.
struct TestHttpServer
{
    EventLoop& loop;
    TcpListener listener{0};
    std::vector<std::shared_ptr<HttpConnection>> conns;

    std::string response_body = "hello";
    int response_code = 200;
    std::string response_text = "OK";
    std::string response_content_type = "text/plain";

    // Last request received
    std::string last_method;
    std::string last_path;
    std::string last_body;

    explicit TestHttpServer(EventLoop& l) : loop(l)
    {
        loop.add_io(listener.fd(), EPOLLIN, [this](uint32_t) {
            auto conn_opt = listener.accept();
            if (!conn_opt) return;

            auto hconn = std::make_shared<HttpConnection>(std::move(*conn_opt));
            int fd = hconn->fd();
            conns.push_back(hconn);

            loop.add_io(fd, EPOLLIN, [this, hconn](uint32_t) {
                bool alive = hconn->on_readable([this](const HttpRequest& req, HttpResponse& resp) {
                    last_method = req.method;
                    last_path   = req.path;
                    last_body   = req.body;

                    resp.set_status(response_code, response_text)
                        .set_body(response_body, response_content_type)
                        .set_close(true);
                });
                if (!alive)
                    loop.remove_io(hconn->fd());
            });
        });
    }

    uint16_t port() const { return listener.local_port(); }
};

// ─── Tests ───────────────────────────────────────────────────────────────────

TEST_CASE("HttpClient: GET to loopback server", "[http_client]")
{
    EventLoop loop;
    TestHttpServer server(loop);

    HttpClientResponse result;
    bool got_response = false;

    HttpClient client(loop);
    auto url = fmt::format("http://127.0.0.1:{}/api/status", server.port());

    client.get(url, {},
        [&](HttpClientResponse r) {
            result = std::move(r);
            got_response = true;
            loop.stop();
        },
        [&](std::string_view) {
            loop.stop();
        });

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_response);
    REQUIRE(result.status_code == 200);
    REQUIRE(result.body == "hello");
    REQUIRE(server.last_method == "GET");
    REQUIRE(server.last_path == "/api/status");
}

TEST_CASE("HttpClient: POST with body", "[http_client]")
{
    EventLoop loop;
    TestHttpServer server(loop);
    server.response_body = "{\"ok\":true}";
    server.response_code = 201;
    server.response_text = "Created";
    server.response_content_type = "application/json";

    HttpClientResponse result;
    bool got_response = false;

    HttpClient client(loop);
    auto url = fmt::format("http://127.0.0.1:{}/api/create", server.port());

    client.post(url, "{\"name\":\"test\"}",
        {{"Content-Type", "application/json"}},
        [&](HttpClientResponse r) {
            result = std::move(r);
            got_response = true;
            loop.stop();
        },
        [&](std::string_view) {
            loop.stop();
        });

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_response);
    REQUIRE(result.status_code == 201);
    REQUIRE(result.body == "{\"ok\":true}");
    REQUIRE(server.last_method == "POST");
    REQUIRE(server.last_path == "/api/create");
    REQUIRE(server.last_body == "{\"name\":\"test\"}");
}

TEST_CASE("HttpClient: error on connection refused", "[http_client]")
{
    EventLoop loop;

    uint16_t port;
    { TcpListener tmp(0); port = tmp.local_port(); }

    bool got_error = false;

    HttpClient client(loop);
    auto url = fmt::format("http://127.0.0.1:{}/", port);

    client.get(url, {},
        [&](HttpClientResponse) { loop.stop(); },
        [&](std::string_view) {
            got_error = true;
            loop.stop();
        });

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_error);
}

TEST_CASE("HttpClient: timeout on unresponsive server", "[http_client]")
{
    EventLoop loop;

    // Server that accepts but never responds
    TcpListener listener(0);
    std::shared_ptr<TcpConnection> held_conn;

    loop.add_io(listener.fd(), EPOLLIN, [&](uint32_t) {
        auto c = listener.accept();
        if (c) held_conn = std::make_shared<TcpConnection>(std::move(*c));
    });

    bool got_error = false;
    std::string error_msg;

    HttpClient client(loop);
    client.set_timeout(300ms);

    auto url = fmt::format("http://127.0.0.1:{}/", listener.local_port());

    client.get(url, {},
        [&](HttpClientResponse) { loop.stop(); },
        [&](std::string_view msg) {
            got_error = true;
            error_msg = std::string(msg);
            loop.stop();
        });

    auto timer = loop.add_timer(3000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    // Either connect timeout or idle timeout should fire
    REQUIRE(got_error);
}

TEST_CASE("HttpClient: URL parsing", "[http_client]")
{
    // Test via a round-trip: send GET with query string
    EventLoop loop;
    TestHttpServer server(loop);

    bool got_response = false;

    HttpClient client(loop);
    auto url = fmt::format("http://127.0.0.1:{}/search?q=hello&page=1", server.port());

    client.get(url, {},
        [&](HttpClientResponse r) {
            got_response = true;
            loop.stop();
        },
        [&](std::string_view) {
            loop.stop();
        });

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_response);
    // Path includes query string
    REQUIRE(server.last_path == "/search");
}
