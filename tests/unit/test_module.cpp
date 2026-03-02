#include <catch2/catch_test_macros.hpp>

#include "apostol/module.hpp"
#include "apostol/http.hpp"
#include "apostol/tcp.hpp"
#include "apostol/event_loop.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace apostol;
using namespace std::chrono_literals;

// ─── Test doubles ─────────────────────────────────────────────────────────────

// A module that always handles any request with a configurable response body.
class StubModule final : public Module
{
public:
    explicit StubModule(std::string name, std::string body, bool enabled = true)
        : enabled_(enabled), name_(std::move(name)), body_(std::move(body)) {}

    std::string_view name()    const override { return name_; }
    bool             enabled() const override { return enabled_; }

    bool execute(const HttpRequest& req, HttpResponse& resp) override
    {
        ++execute_count;
        last_path = req.path;
        if (!handles_)
            return false;
        resp.set_status(200, "OK").set_body(body_);
        return true;
    }

    void heartbeat(std::chrono::system_clock::time_point) override
    {
        ++heartbeat_count;
    }

    // Configurable behaviour
    bool handles_ {true};
    bool enabled_;

    // Observation
    int         execute_count  {0};
    int         heartbeat_count{0};
    std::string last_path;

private:
    std::string name_;
    std::string body_;  // declared after enabled_ to match constructor order
};

// Helper: connect a raw blocking socket to 127.0.0.1:port
static int connect_to(uint16_t port)
{
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    REQUIRE(fd >= 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    REQUIRE(::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    return fd;
}

static std::string read_response(int fd, int timeout_ms = 500)
{
    struct timeval tv{ .tv_sec = 0, .tv_usec = timeout_ms * 1000 };
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    std::string result;
    char buf[1024];
    for (;;) {
        ssize_t n = ::read(fd, buf, sizeof(buf));
        if (n <= 0) break;
        result.append(buf, static_cast<std::size_t>(n));
    }
    return result;
}

// ─── Module interface ─────────────────────────────────────────────────────────

TEST_CASE("Module: enabled() controls whether it participates in dispatch", "[module]")
{
    StubModule enabled_mod("a", "body", /*enabled=*/true);
    StubModule disabled_mod("b", "body", /*enabled=*/false);

    REQUIRE(enabled_mod.enabled());
    REQUIRE_FALSE(disabled_mod.enabled());
}

TEST_CASE("Module: name() returns module identifier", "[module]")
{
    StubModule mod("my-module", "");
    REQUIRE(mod.name() == "my-module");
}

// ─── ModuleManager: execute ───────────────────────────────────────────────────

TEST_CASE("ModuleManager: execute returns false when no modules registered", "[module]")
{
    ModuleManager mgr;
    HttpRequest req;
    HttpResponse resp;
    REQUIRE_FALSE(mgr.execute(req, resp));
}

TEST_CASE("ModuleManager: execute returns false when all modules are disabled", "[module]")
{
    ModuleManager mgr;
    mgr.add_module(std::make_unique<StubModule>("a", "body", /*enabled=*/false));
    mgr.add_module(std::make_unique<StubModule>("b", "body", /*enabled=*/false));

    HttpRequest req;
    HttpResponse resp;
    REQUIRE_FALSE(mgr.execute(req, resp));
}

TEST_CASE("ModuleManager: execute returns false when no module handles request", "[module]")
{
    ModuleManager mgr;
    auto* m = new StubModule("a", "body");
    m->handles_ = false;
    mgr.add_module(std::unique_ptr<Module>(m));

    HttpRequest req;
    HttpResponse resp;
    REQUIRE_FALSE(mgr.execute(req, resp));
    REQUIRE(m->execute_count == 1);  // was called but returned false
}

TEST_CASE("ModuleManager: enabled module executes and its response is used", "[module]")
{
    ModuleManager mgr;
    mgr.add_module(std::make_unique<StubModule>("echo", "hello"));

    HttpRequest req;
    req.path = "/test";
    HttpResponse resp;

    REQUIRE(mgr.execute(req, resp));
    REQUIRE(resp.serialize().find("hello") != std::string::npos);
}

TEST_CASE("ModuleManager: execute passes request path to module", "[module]")
{
    ModuleManager mgr;
    auto* m = new StubModule("spy", "ok");
    mgr.add_module(std::unique_ptr<Module>(m));

    HttpRequest req;
    req.path = "/api/users";
    HttpResponse resp;

    mgr.execute(req, resp);
    REQUIRE(m->last_path == "/api/users");
}

TEST_CASE("ModuleManager: first handling module short-circuits, second not called", "[module]")
{
    ModuleManager mgr;
    auto* first  = new StubModule("first",  "first");   // handles_=true
    auto* second = new StubModule("second", "second");  // handles_=true

    mgr.add_module(std::unique_ptr<Module>(first));
    mgr.add_module(std::unique_ptr<Module>(second));

    HttpRequest req;
    HttpResponse resp;

    REQUIRE(mgr.execute(req, resp));
    REQUIRE(first->execute_count  == 1);
    REQUIRE(second->execute_count == 0);  // never reached
    REQUIRE(resp.serialize().find("first") != std::string::npos);
}

TEST_CASE("ModuleManager: disabled module is skipped, enabled module handles", "[module]")
{
    ModuleManager mgr;
    auto* disabled = new StubModule("off", "off",  /*enabled=*/false);
    auto* enabled  = new StubModule("on",  "on",   /*enabled=*/true);

    mgr.add_module(std::unique_ptr<Module>(disabled));
    mgr.add_module(std::unique_ptr<Module>(enabled));

    HttpRequest req;
    HttpResponse resp;

    REQUIRE(mgr.execute(req, resp));
    REQUIRE(disabled->execute_count == 0);  // skipped
    REQUIRE(enabled->execute_count  == 1);
}

TEST_CASE("ModuleManager: count() reflects number of registered modules", "[module]")
{
    ModuleManager mgr;
    REQUIRE(mgr.count() == 0);
    mgr.add_module(std::make_unique<StubModule>("a", ""));
    REQUIRE(mgr.count() == 1);
    mgr.add_module(std::make_unique<StubModule>("b", ""));
    REQUIRE(mgr.count() == 2);
}

// ─── ModuleManager: heartbeat ─────────────────────────────────────────────────

TEST_CASE("ModuleManager: heartbeat is called on all enabled modules", "[module]")
{
    ModuleManager mgr;
    auto* a = new StubModule("a", "");
    auto* b = new StubModule("b", "");
    mgr.add_module(std::unique_ptr<Module>(a));
    mgr.add_module(std::unique_ptr<Module>(b));

    auto now = std::chrono::system_clock::now();
    mgr.heartbeat(now);

    REQUIRE(a->heartbeat_count == 1);
    REQUIRE(b->heartbeat_count == 1);
}

TEST_CASE("ModuleManager: heartbeat skips disabled modules", "[module]")
{
    ModuleManager mgr;
    auto* on  = new StubModule("on",  "", /*enabled=*/true);
    auto* off = new StubModule("off", "", /*enabled=*/false);
    mgr.add_module(std::unique_ptr<Module>(on));
    mgr.add_module(std::unique_ptr<Module>(off));

    mgr.heartbeat(std::chrono::system_clock::now());

    REQUIRE(on->heartbeat_count  == 1);
    REQUIRE(off->heartbeat_count == 0);
}

TEST_CASE("ModuleManager: heartbeat called multiple times accumulates", "[module]")
{
    ModuleManager mgr;
    auto* m = new StubModule("m", "");
    mgr.add_module(std::unique_ptr<Module>(m));

    auto now = std::chrono::system_clock::now();
    mgr.heartbeat(now);
    mgr.heartbeat(now);
    mgr.heartbeat(now);

    REQUIRE(m->heartbeat_count == 3);
}

// ─── Integration: ModuleManager + HttpConnection + EventLoop ─────────────────

TEST_CASE("ModuleManager + HttpConnection: dispatches HTTP request to module", "[module]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    ModuleManager mgr;
    mgr.add_module(std::make_unique<StubModule>("ping", "pong"));

    std::string raw_response;
    int client_fd = -1;

    loop.add_io(listener.fd(), EPOLLIN, [&](uint32_t) {
        auto conn_opt = listener.accept();
        REQUIRE(conn_opt.has_value());

        auto http_conn = std::make_shared<HttpConnection>(std::move(*conn_opt));
        int conn_fd = http_conn->fd();

        loop.add_io(conn_fd, EPOLLIN | EPOLLRDHUP, [&loop, &mgr, http_conn](uint32_t) {
            bool keep = http_conn->on_readable([&mgr](const HttpRequest& req, HttpResponse& resp) {
                if (!mgr.execute(req, resp))
                    resp.set_status(404, "Not Found").set_body("not found");
            });
            if (!keep)
                loop.stop();
        });
    });

    loop.add_timer(10ms, [&] {
        client_fd = connect_to(port);
        std::string raw =
            "GET /ping HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n"
            "\r\n";
        (void)::write(client_fd, raw.data(), raw.size());
    }, false);

    loop.add_timer(300ms, [&] { loop.stop(); }, false);
    loop.run();

    if (client_fd >= 0) {
        raw_response = read_response(client_fd);
        ::close(client_fd);
    }

    REQUIRE(raw_response.find("HTTP/1.1 200 OK") != std::string::npos);
    REQUIRE(raw_response.find("pong") != std::string::npos);
}

TEST_CASE("ModuleManager + HttpConnection: returns 404 when no module handles", "[module]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    ModuleManager mgr;
    auto* m = new StubModule("picky", "");
    m->handles_ = false;
    mgr.add_module(std::unique_ptr<Module>(m));

    std::string raw_response;
    int client_fd = -1;

    loop.add_io(listener.fd(), EPOLLIN, [&](uint32_t) {
        auto conn_opt = listener.accept();
        REQUIRE(conn_opt.has_value());
        auto http_conn = std::make_shared<HttpConnection>(std::move(*conn_opt));
        int conn_fd    = http_conn->fd();

        loop.add_io(conn_fd, EPOLLIN | EPOLLRDHUP, [&loop, &mgr, http_conn](uint32_t) {
            bool keep = http_conn->on_readable([&mgr](const HttpRequest& req, HttpResponse& resp) {
                if (!mgr.execute(req, resp))
                    resp.set_status(404, "Not Found").set_body("not found");
            });
            if (!keep) loop.stop();
        });
    });

    loop.add_timer(10ms, [&] {
        client_fd = connect_to(port);
        std::string raw = "GET /anything HTTP/1.1\r\nHost: h\r\nConnection: close\r\n\r\n";
        (void)::write(client_fd, raw.data(), raw.size());
    }, false);

    loop.add_timer(300ms, [&] { loop.stop(); }, false);
    loop.run();

    if (client_fd >= 0) {
        raw_response = read_response(client_fd);
        ::close(client_fd);
    }

    REQUIRE(raw_response.find("HTTP/1.1 404 Not Found") != std::string::npos);
}

// ─── Application integration: start_http_server ──────────────────────────────

// We test start_http_server() by sub-classing Application directly and
// running only the worker loop logic (not the full fork/exec lifecycle).

#include "apostol/application.hpp"

class TestApp final : public Application
{
public:
    TestApp() : Application("test") {}

    uint16_t bound_port{0};
    std::string last_response;

protected:
    void on_worker_start(EventLoop& loop) override
    {
        module_manager().add_module(
            std::make_unique<StubModule>("greet", "hello from app"));

        start_http_server(loop, /*port=*/0);
        bound_port = http_port();

        // Self-stop after a short time — we just need to verify setup
        loop.add_timer(300ms, [&] { loop.stop(); }, false);
    }
};

TEST_CASE("Application::start_http_server: HTTP server serves module responses", "[module]")
{
    TestApp app;

    // Run worker_run() directly (bypasses fork/master).
    // worker_run() resets the signal mask, so we call the inner body manually.
    EventLoop loop;

    app.module_manager().add_module(
        std::make_unique<StubModule>("hello", "hello world"));

    app.start_http_server(loop, /*port=*/0);
    uint16_t port = app.http_port();
    REQUIRE(port > 0);

    std::string raw_response;
    int client_fd = -1;

    loop.add_timer(10ms, [&] {
        client_fd = connect_to(port);
        std::string raw =
            "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
        (void)::write(client_fd, raw.data(), raw.size());
    }, false);

    loop.add_timer(300ms, [&] { loop.stop(); }, false);
    loop.run();

    if (client_fd >= 0) {
        raw_response = read_response(client_fd);
        ::close(client_fd);
    }

    REQUIRE(raw_response.find("hello world") != std::string::npos);
}
