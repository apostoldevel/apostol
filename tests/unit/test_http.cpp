#include <catch2/catch_test_macros.hpp>

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

using namespace apostol;
using namespace std::chrono_literals;

// ─── Helpers ──────────────────────────────────────────────────────────────────

static int connect_to(uint16_t port)
{
    // Use IPv4 127.0.0.1; dual-stack TcpListener accepts it via IPv4-mapped address
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    REQUIRE(fd >= 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    REQUIRE(::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    return fd;
}

// Read everything from fd until timeout or EOF (blocking with SO_RCVTIMEO)
static std::string read_all(int fd, int timeout_ms = 500)
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

// ─── HttpRequest tests ────────────────────────────────────────────────────────

TEST_CASE("HttpParser: parses a simple GET request", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    int calls = 0;

    parser.set_handler([&](HttpRequest r) {
        req = std::move(r);
        ++calls;
    });

    std::string raw =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    REQUIRE(parser.feed(raw.data(), raw.size()));
    REQUIRE(calls == 1);
    REQUIRE(req.method  == "GET");
    REQUIRE(req.path    == "/hello");
    REQUIRE(req.version == "HTTP/1.1");
}

TEST_CASE("HttpParser: parses a POST request with body", "[http]")
{
    HttpParser parser;
    HttpRequest req;

    parser.set_handler([&](HttpRequest r) { req = std::move(r); });

    std::string raw =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 7\r\n"
        "\r\n"
        "payload";

    REQUIRE(parser.feed(raw.data(), raw.size()));
    REQUIRE(req.method == "POST");
    REQUIRE(req.path   == "/submit");
    REQUIRE(req.body   == "payload");
}

TEST_CASE("HttpParser: parses request fed in small chunks", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    int calls = 0;

    parser.set_handler([&](HttpRequest r) {
        req = std::move(r);
        ++calls;
    });

    std::string raw =
        "GET /chunked HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";

    // Feed one byte at a time
    for (char c : raw) {
        REQUIRE(parser.feed(&c, 1));
    }

    REQUIRE(calls == 1);
    REQUIRE(req.path == "/chunked");
}

TEST_CASE("HttpParser: parses two pipelined requests", "[http]")
{
    HttpParser parser;
    int calls = 0;
    std::vector<std::string> paths;

    parser.set_handler([&](HttpRequest r) {
        paths.push_back(r.path);
        ++calls;
    });

    std::string raw =
        "GET /first HTTP/1.1\r\nHost: h\r\n\r\n"
        "GET /second HTTP/1.1\r\nHost: h\r\n\r\n";

    REQUIRE(parser.feed(raw.data(), raw.size()));
    REQUIRE(calls == 2);
    REQUIRE(paths[0] == "/first");
    REQUIRE(paths[1] == "/second");
}

TEST_CASE("HttpParser: returns false on malformed request", "[http]")
{
    HttpParser parser;
    parser.set_handler([](HttpRequest) {});

    std::string bad = "NOT VALID HTTP\r\n\r\n";
    REQUIRE_FALSE(parser.feed(bad.data(), bad.size()));
}

// ─── HttpRequest helpers ──────────────────────────────────────────────────────

TEST_CASE("HttpRequest: header() lookup is case-insensitive", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    parser.set_handler([&](HttpRequest r) { req = std::move(r); });

    std::string raw =
        "GET / HTTP/1.1\r\n"
        "Content-Type: application/json\r\n"
        "X-Custom: value\r\n"
        "\r\n";
    parser.feed(raw.data(), raw.size());

    REQUIRE(req.header("content-type")   == "application/json");
    REQUIRE(req.header("CONTENT-TYPE")   == "application/json");
    REQUIRE(req.header("Content-Type")   == "application/json");
    REQUIRE(req.header("x-custom")       == "value");
    REQUIRE(req.header("X-Missing")      == "");  // missing → empty
}

TEST_CASE("HttpRequest: keep_alive() is true for HTTP/1.1 by default", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    parser.set_handler([&](HttpRequest r) { req = std::move(r); });

    std::string raw = "GET / HTTP/1.1\r\nHost: h\r\n\r\n";
    parser.feed(raw.data(), raw.size());
    REQUIRE(req.keep_alive());
}

TEST_CASE("HttpRequest: keep_alive() is false when Connection: close", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    parser.set_handler([&](HttpRequest r) { req = std::move(r); });

    std::string raw = "GET / HTTP/1.1\r\nHost: h\r\nConnection: close\r\n\r\n";
    parser.feed(raw.data(), raw.size());
    REQUIRE_FALSE(req.keep_alive());
}

TEST_CASE("HttpRequest: keep_alive() is false for HTTP/1.0", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    parser.set_handler([&](HttpRequest r) { req = std::move(r); });

    std::string raw = "GET / HTTP/1.0\r\nHost: h\r\n\r\n";
    parser.feed(raw.data(), raw.size());
    REQUIRE_FALSE(req.keep_alive());
}

// ─── HttpResponse tests ───────────────────────────────────────────────────────

TEST_CASE("HttpResponse: serialize produces status line and CRLF body separator", "[http]")
{
    HttpResponse resp;
    resp.set_status(200, "OK");
    resp.set_body("Hello");

    std::string s = resp.serialize();
    REQUIRE(s.starts_with("HTTP/1.1 200 OK\r\n"));
    REQUIRE(s.find("\r\n\r\n") != std::string::npos);
    REQUIRE(s.ends_with("Hello"));
}

TEST_CASE("HttpResponse: serialize includes Content-Length automatically", "[http]")
{
    HttpResponse resp;
    resp.set_body("Hi");
    std::string s = resp.serialize();
    REQUIRE(s.find("Content-Length: 2\r\n") != std::string::npos);
}

TEST_CASE("HttpResponse: set_body sets Content-Type header", "[http]")
{
    HttpResponse resp;
    resp.set_body("{}", "application/json");
    std::string s = resp.serialize();
    REQUIRE(s.find("Content-Type: application/json\r\n") != std::string::npos);
}

TEST_CASE("HttpResponse: custom header is included in output", "[http]")
{
    HttpResponse resp;
    resp.set_header("X-Powered-By", "apostol");
    std::string s = resp.serialize();
    REQUIRE(s.find("X-Powered-By: apostol\r\n") != std::string::npos);
}

TEST_CASE("HttpResponse: 404 status is serialized correctly", "[http]")
{
    HttpResponse resp;
    resp.set_status(404, "Not Found");
    std::string s = resp.serialize();
    REQUIRE(s.starts_with("HTTP/1.1 404 Not Found\r\n"));
}

// ─── HttpConnection + EventLoop integration ───────────────────────────────────

TEST_CASE("HttpConnection: end-to-end GET request and response", "[http]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    std::string req_path;
    std::string raw_response;
    int client_fd = -1;

    loop.add_io(listener.fd(), EPOLLIN, [&](uint32_t) {
        auto conn_opt = listener.accept();
        REQUIRE(conn_opt.has_value());

        auto http_conn = std::make_shared<HttpConnection>(std::move(*conn_opt));
        int conn_fd = http_conn->fd();

        loop.add_io(conn_fd, EPOLLIN | EPOLLRDHUP, [&loop, &req_path, http_conn](uint32_t) {
            bool keep_open = http_conn->on_readable([&req_path](const HttpRequest& req, HttpResponse& resp) {
                req_path = req.path;
                resp.set_status(200, "OK");
                resp.set_body("pong", "text/plain");
            });
            if (!keep_open)
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
    }, /*repeat=*/false);

    // Stop the loop after client side gets a response
    loop.add_timer(200ms, [&] { loop.stop(); }, /*repeat=*/false);

    loop.run();

    // Read response from client side
    if (client_fd >= 0) {
        raw_response = read_all(client_fd);
        ::close(client_fd);
    }

    REQUIRE(req_path == "/ping");
    REQUIRE(raw_response.find("HTTP/1.1 200 OK") != std::string::npos);
    REQUIRE(raw_response.find("pong") != std::string::npos);
}

// ─── HttpRequest: query string and params ────────────────────────────────────

TEST_CASE("HttpRequest: query string is separated from path", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    parser.set_handler([&](HttpRequest r) { req = std::move(r); });

    std::string raw = "GET /search?q=hello+world&page=2 HTTP/1.1\r\nHost: h\r\n\r\n";
    parser.feed(raw.data(), raw.size());

    REQUIRE(req.path  == "/search");
    REQUIRE(req.query == "q=hello+world&page=2");
    REQUIRE(req.params.size() == 2);
    REQUIRE(req.param("q")            == "hello world");
    REQUIRE(req.param("page")         == "2");
    REQUIRE(req.param("missing", "x") == "x");
}

TEST_CASE("HttpRequest: URL-encoded params are decoded", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    parser.set_handler([&](HttpRequest r) { req = std::move(r); });

    std::string raw = "GET /api?name=John%20Doe&token=a%2Bb HTTP/1.1\r\nHost: h\r\n\r\n";
    parser.feed(raw.data(), raw.size());

    REQUIRE(req.param("name")  == "John Doe");
    REQUIRE(req.param("token") == "a+b");
}

TEST_CASE("HttpRequest: path without query string is unchanged", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    parser.set_handler([&](HttpRequest r) { req = std::move(r); });

    std::string raw = "GET /plain HTTP/1.1\r\nHost: h\r\n\r\n";
    parser.feed(raw.data(), raw.size());

    REQUIRE(req.path  == "/plain");
    REQUIRE(req.query == "");
    REQUIRE(req.params.empty());
}

// ─── HttpRequest: cookies ─────────────────────────────────────────────────────

TEST_CASE("HttpRequest: cookies are parsed from Cookie header", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    parser.set_handler([&](HttpRequest r) { req = std::move(r); });

    std::string raw =
        "GET / HTTP/1.1\r\n"
        "Host: h\r\n"
        "Cookie: session=abc123; user=alice\r\n"
        "\r\n";
    parser.feed(raw.data(), raw.size());

    REQUIRE(req.cookie("session")           == "abc123");
    REQUIRE(req.cookie("user")              == "alice");
    REQUIRE(req.cookie("missing", "default") == "default");
}

TEST_CASE("HttpRequest: no Cookie header yields empty cookies", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    parser.set_handler([&](HttpRequest r) { req = std::move(r); });

    std::string raw = "GET / HTTP/1.1\r\nHost: h\r\n\r\n";
    parser.feed(raw.data(), raw.size());

    REQUIRE(req.cookies.empty());
}

// ─── HttpRequest: content helpers ────────────────────────────────────────────

TEST_CASE("HttpRequest: content_type() and content_length() helpers", "[http]")
{
    HttpParser parser;
    HttpRequest req;
    parser.set_handler([&](HttpRequest r) { req = std::move(r); });

    std::string raw =
        "POST /api HTTP/1.1\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 2\r\n"
        "\r\n{}";
    parser.feed(raw.data(), raw.size());

    REQUIRE(req.content_type()   == "application/json");
    REQUIRE(req.content_length() == 2);
}

// ─── HttpStatus enum ─────────────────────────────────────────────────────────

TEST_CASE("HttpStatus: set_status(HttpStatus) uses standard reason phrase", "[http]")
{
    HttpResponse resp;
    resp.set_status(HttpStatus::not_found);
    REQUIRE(resp.serialize().starts_with("HTTP/1.1 404 Not Found\r\n"));

    resp.clear();
    resp.set_status(HttpStatus::ok);
    REQUIRE(resp.serialize().starts_with("HTTP/1.1 200 OK\r\n"));

    resp.clear();
    resp.set_status(HttpStatus::created);
    REQUIRE(resp.serialize().starts_with("HTTP/1.1 201 Created\r\n"));

    resp.clear();
    resp.set_status(HttpStatus::internal_server_error);
    REQUIRE(resp.serialize().starts_with("HTTP/1.1 500 Internal Server Error\r\n"));
}

TEST_CASE("HttpStatus: status_text() free function returns correct phrase", "[http]")
{
    REQUIRE(status_text(HttpStatus::ok)           == "OK");
    REQUIRE(status_text(HttpStatus::not_found)    == "Not Found");
    REQUIRE(status_text(HttpStatus::not_allowed)  == "Method Not Allowed");
    REQUIRE(status_text(HttpStatus::unauthorized) == "Unauthorized");
}

// ─── HttpResponse new methods ────────────────────────────────────────────────

TEST_CASE("HttpResponse: del_header removes a header", "[http]")
{
    HttpResponse resp;
    resp.set_header("X-Foo", "bar");
    REQUIRE(resp.serialize().find("X-Foo") != std::string::npos);
    resp.del_header("X-Foo");
    REQUIRE(resp.serialize().find("X-Foo") == std::string::npos);
}

TEST_CASE("HttpResponse: clear resets to 200 OK with empty body", "[http]")
{
    HttpResponse resp;
    resp.set_status(500, "Internal Server Error");
    resp.set_body("error");
    resp.clear();

    std::string s = resp.serialize();
    REQUIRE(s.starts_with("HTTP/1.1 200 OK\r\n"));
    REQUIRE(s.find("Content-Length: 0\r\n") != std::string::npos);
    REQUIRE(!s.ends_with("error"));
}

TEST_CASE("HttpResponse: set_close adds Connection: close", "[http]")
{
    HttpResponse resp;
    resp.set_close();
    REQUIRE(resp.serialize().find("Connection: close\r\n") != std::string::npos);
}

TEST_CASE("HttpResponse: set_cookie produces Set-Cookie header", "[http]")
{
    HttpResponse resp;
    resp.set_cookie("session", "tok123", "/", 3600, true, "Lax");

    std::string s = resp.serialize();
    REQUIRE(s.find("Set-Cookie: session=tok123; Path=/; Max-Age=3600; HttpOnly; SameSite=Lax\r\n")
            != std::string::npos);
}

TEST_CASE("HttpResponse: set_cookie with secure and domain flags", "[http]")
{
    HttpResponse resp;
    resp.set_cookie("auth", "xyz", "/", 0, true, "Strict", true, "example.com");

    std::string s = resp.serialize();
    REQUIRE(s.find("Set-Cookie: auth=xyz; Path=/; Domain=example.com; HttpOnly; SameSite=Strict; Secure\r\n")
            != std::string::npos);
}

TEST_CASE("HttpResponse: add_header allows duplicate headers", "[http]")
{
    HttpResponse resp;
    resp.add_header("X-Multi", "first");
    resp.add_header("X-Multi", "second");

    std::string s = resp.serialize();
    REQUIRE(s.find("X-Multi: first\r\n")  != std::string::npos);
    REQUIRE(s.find("X-Multi: second\r\n") != std::string::npos);
}

TEST_CASE("HttpResponse: set_header replaces, add_header appends", "[http]")
{
    HttpResponse resp;
    resp.set_header("X-Single", "one");
    resp.set_header("X-Single", "two");  // replaces

    std::string s = resp.serialize();
    REQUIRE(s.find("X-Single: two\r\n")   != std::string::npos);
    REQUIRE(s.find("X-Single: one\r\n")   == std::string::npos);
}

// ─── HttpConnection + EventLoop integration ───────────────────────────────────

TEST_CASE("HttpConnection: 404 response for unknown path", "[http]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    std::string raw_response;
    int client_fd = -1;

    loop.add_io(listener.fd(), EPOLLIN, [&](uint32_t) {
        auto conn_opt = listener.accept();
        REQUIRE(conn_opt.has_value());

        auto http_conn = std::make_shared<HttpConnection>(std::move(*conn_opt));
        int conn_fd = http_conn->fd();

        loop.add_io(conn_fd, EPOLLIN | EPOLLRDHUP, [&loop, http_conn](uint32_t) {
            http_conn->on_readable([](const HttpRequest& req, HttpResponse& resp) {
                resp.set_status(404, "Not Found");
                resp.set_body("not found");
            });
            loop.stop();
        });
    });

    loop.add_timer(10ms, [&] {
        client_fd = connect_to(port);
        std::string raw = "GET /missing HTTP/1.1\r\nHost: h\r\nConnection: close\r\n\r\n";
        (void)::write(client_fd, raw.data(), raw.size());
    }, /*repeat=*/false);

    loop.add_timer(200ms, [&] { loop.stop(); }, /*repeat=*/false);
    loop.run();

    if (client_fd >= 0) {
        raw_response = read_all(client_fd);
        ::close(client_fd);
    }

    REQUIRE(raw_response.find("HTTP/1.1 404 Not Found") != std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════════
// HttpResponseParser tests
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("HttpResponseParser: parse simple 200 OK", "[http][HttpResponseParser]")
{
    HttpResponseParser parser;
    HttpClientResponse resp;
    bool called = false;

    parser.set_handler([&](HttpClientResponse r) {
        resp = std::move(r);
        called = true;
    });

    std::string raw =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello";

    REQUIRE(parser.feed(raw.data(), raw.size()));
    REQUIRE(called);
    REQUIRE(resp.status_code == 200);
    REQUIRE(resp.status_text == "OK");
    REQUIRE(resp.version == "HTTP/1.1");
    REQUIRE(resp.body == "hello");
    REQUIRE(resp.header("Content-Type") == "text/plain");
    REQUIRE(resp.content_length() == 5);
}

TEST_CASE("HttpResponseParser: parse response with Content-Length", "[http][HttpResponseParser]")
{
    HttpResponseParser parser;
    HttpClientResponse resp;

    parser.set_handler([&](HttpClientResponse r) {
        resp = std::move(r);
    });

    std::string raw =
        "HTTP/1.1 201 Created\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 12\r\n"
        "\r\n"
        "{\"id\":\"abc\"}";

    REQUIRE(parser.feed(raw.data(), raw.size()));
    REQUIRE(resp.status_code == 201);
    REQUIRE(resp.status_text == "Created");
    REQUIRE(resp.body == "{\"id\":\"abc\"}");
}

TEST_CASE("HttpResponseParser: parse chunked response", "[http][HttpResponseParser]")
{
    HttpResponseParser parser;
    HttpClientResponse resp;
    bool called = false;

    parser.set_handler([&](HttpClientResponse r) {
        resp = std::move(r);
        called = true;
    });

    std::string raw =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "hello\r\n"
        "6\r\n"
        " world\r\n"
        "0\r\n"
        "\r\n";

    REQUIRE(parser.feed(raw.data(), raw.size()));
    REQUIRE(called);
    REQUIRE(resp.body == "hello world");
}

TEST_CASE("HttpResponseParser: parse 204 No Content (no body)", "[http][HttpResponseParser]")
{
    HttpResponseParser parser;
    HttpClientResponse resp;
    bool called = false;

    parser.set_handler([&](HttpClientResponse r) {
        resp = std::move(r);
        called = true;
    });

    std::string raw =
        "HTTP/1.1 204 No Content\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    REQUIRE(parser.feed(raw.data(), raw.size()));
    REQUIRE(called);
    REQUIRE(resp.status_code == 204);
    REQUIRE(resp.body.empty());
}

TEST_CASE("HttpResponseParser: parse 304 Not Modified (no body)", "[http][HttpResponseParser]")
{
    HttpResponseParser parser;
    HttpClientResponse resp;
    bool called = false;

    parser.set_handler([&](HttpClientResponse r) {
        resp = std::move(r);
        called = true;
    });

    std::string raw =
        "HTTP/1.1 304 Not Modified\r\n"
        "ETag: \"abc123\"\r\n"
        "\r\n";

    REQUIRE(parser.feed(raw.data(), raw.size()));
    REQUIRE(called);
    REQUIRE(resp.status_code == 304);
    REQUIRE(resp.body.empty());
    REQUIRE(resp.header("ETag") == "\"abc123\"");
}

TEST_CASE("HttpResponseParser: multi-response feed (keep-alive)", "[http][HttpResponseParser]")
{
    HttpResponseParser parser;
    int count = 0;

    parser.set_handler([&](HttpClientResponse r) {
        ++count;
        REQUIRE(r.status_code == 200);
        REQUIRE(r.body == "ok");
    });

    std::string raw =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 2\r\n"
        "\r\n"
        "ok"
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 2\r\n"
        "\r\n"
        "ok";

    REQUIRE(parser.feed(raw.data(), raw.size()));
    REQUIRE(count == 2);
}

TEST_CASE("HttpResponseParser: incremental feed across multiple calls", "[http][HttpResponseParser]")
{
    HttpResponseParser parser;
    HttpClientResponse resp;
    bool called = false;

    parser.set_handler([&](HttpClientResponse r) {
        resp = std::move(r);
        called = true;
    });

    std::string part1 = "HTTP/1.1 200 OK\r\nContent-Len";
    std::string part2 = "gth: 3\r\n\r\nfoo";

    REQUIRE(parser.feed(part1.data(), part1.size()));
    REQUIRE_FALSE(called);

    REQUIRE(parser.feed(part2.data(), part2.size()));
    REQUIRE(called);
    REQUIRE(resp.body == "foo");
}

TEST_CASE("HttpClientResponse: case-insensitive header lookup", "[http][HttpResponseParser]")
{
    HttpClientResponse resp;
    resp.headers = {{"Content-Type", "text/html"}, {"X-Custom", "value"}};

    REQUIRE(resp.header("content-type") == "text/html");
    REQUIRE(resp.header("CONTENT-TYPE") == "text/html");
    REQUIRE(resp.header("x-custom") == "value");
    REQUIRE(resp.header("nonexistent").empty());
}
