#include <catch2/catch_test_macros.hpp>

#include "apostol/http_proxy.hpp"
#include "apostol/http.hpp"
#include "apostol/tcp.hpp"
#include "apostol/event_loop.hpp"

#include <chrono>
#include <memory>
#include <string>

using namespace apostol;
using namespace std::chrono_literals;

// ─── Helper: upstream server ─────────────────────────────────────────────────

struct UpstreamServer
{
    EventLoop& loop;
    TcpListener listener{0};
    std::vector<std::shared_ptr<HttpConnection>> conns;

    std::string response_body = "upstream ok";
    int response_code = 200;
    std::string response_text = "OK";

    std::string last_method;
    std::string last_path;
    std::string last_body;
    std::string last_x_forwarded_for;

    explicit UpstreamServer(EventLoop& l) : loop(l)
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
                    last_x_forwarded_for = req.header("X-Forwarded-For");

                    resp.set_status(response_code, response_text)
                        .set_body(response_body, "text/plain")
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

TEST_CASE("HttpProxy: forward GET to upstream", "[http_proxy]")
{
    EventLoop loop;
    UpstreamServer upstream(loop);

    HttpProxy proxy(loop, "127.0.0.1", upstream.port());

    // Simulate an incoming server-side request
    HttpRequest req;
    req.method  = "GET";
    req.path    = "/api/data";
    req.version = "HTTP/1.1";
    req.headers = {{"Accept", "application/json"}};
    req.peer_ip = "10.0.0.1";

    HttpResponse proxy_response;
    bool got_response = false;

    proxy.forward(req,
        [&](const HttpResponse& resp) {
            proxy_response = resp;
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
    auto serialized = proxy_response.serialize();
    REQUIRE(serialized.find("200 OK") != std::string::npos);
    REQUIRE(serialized.find("upstream ok") != std::string::npos);
    REQUIRE(upstream.last_method == "GET");
    REQUIRE(upstream.last_path == "/api/data");
    REQUIRE(upstream.last_x_forwarded_for == "10.0.0.1");
}

TEST_CASE("HttpProxy: forward POST with body", "[http_proxy]")
{
    EventLoop loop;
    UpstreamServer upstream(loop);
    upstream.response_body = "{\"created\":true}";
    upstream.response_code = 201;
    upstream.response_text = "Created";

    HttpProxy proxy(loop, "127.0.0.1", upstream.port());

    HttpRequest req;
    req.method  = "POST";
    req.path    = "/api/items";
    req.version = "HTTP/1.1";
    req.headers = {{"Content-Type", "application/json"}};
    req.body    = "{\"name\":\"foo\"}";

    bool got_response = false;
    HttpResponse proxy_response;

    proxy.forward(req,
        [&](const HttpResponse& resp) {
            proxy_response = resp;
            got_response = true;
            loop.stop();
        });

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_response);
    auto serialized = proxy_response.serialize();
    REQUIRE(serialized.find("201 Created") != std::string::npos);
    REQUIRE(upstream.last_body == "{\"name\":\"foo\"}");
}

TEST_CASE("HttpProxy: error on unreachable upstream", "[http_proxy]")
{
    EventLoop loop;

    uint16_t port;
    { TcpListener tmp(0); port = tmp.local_port(); }

    HttpProxy proxy(loop, "127.0.0.1", port);

    HttpRequest req;
    req.method = "GET";
    req.path   = "/";

    bool got_error = false;

    proxy.forward(req,
        [&](const HttpResponse&) { loop.stop(); },
        [&](std::string_view) {
            got_error = true;
            loop.stop();
        });

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_error);
}
