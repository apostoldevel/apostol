#include <catch2/catch_test_macros.hpp>

#include "apostol/tcp_client.hpp"
#include "apostol/tcp.hpp"
#include "apostol/event_loop.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <memory>
#include <string>

using namespace apostol;
using namespace std::chrono_literals;

// ─── Helpers ─────────────────────────────────────────────────────────────────

// Simple echo server: accepts one connection, echoes data back, then closes.
static void run_echo_server(EventLoop& loop, TcpListener& listener)
{
    loop.add_io(listener.fd(), EPOLLIN, [&loop, &listener](uint32_t) {
        auto conn_opt = listener.accept();
        if (!conn_opt) return;

        auto conn = std::make_shared<TcpConnection>(std::move(*conn_opt));
        int conn_fd = conn->fd();

        loop.add_io(conn_fd, EPOLLIN, [&loop, conn](uint32_t) {
            char buf[4096];
            ssize_t n = conn->read(buf, sizeof(buf));
            if (n > 0) {
                conn->write(buf, static_cast<size_t>(n));
            } else if (n == 0) {
                loop.remove_io(conn->fd());
            }
        });
    });
}

// ─── Smoke tests ─────────────────────────────────────────────────────────────

TEST_CASE("TcpClient: construct and destruct without connect", "[tcp_client]")
{
    EventLoop loop;
    TcpClient client(loop);

    REQUIRE(client.state() == TcpClientState::Idle);
    REQUIRE(client.fd() == -1);
    REQUIRE_FALSE(client.connected());
}

// ─── Connect tests ──────────────────────────────────────────────────────────

TEST_CASE("TcpClient: connect to loopback listener", "[tcp_client]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    bool connected = false;
    TcpClient client(loop);
    client.on_connect([&] {
        connected = true;
        loop.stop();
    });
    client.on_error([&](std::string_view) {
        loop.stop();
    });

    client.connect("127.0.0.1", port);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(connected);
    REQUIRE(client.connected());
}

TEST_CASE("TcpClient: send and receive via loopback echo", "[tcp_client]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    run_echo_server(loop, listener);

    std::string received;
    TcpClient client(loop);

    client.on_connect([&] {
        client.send("hello world");
    });
    client.on_data([&](const char* data, size_t len) {
        received.append(data, len);
        if (received.size() >= 11)
            loop.stop();
    });
    client.on_error([&](std::string_view) {
        loop.stop();
    });

    client.connect("127.0.0.1", port);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(received == "hello world");
}

TEST_CASE("TcpClient: partial write buffering with large data", "[tcp_client]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    run_echo_server(loop, listener);

    // 256KB — likely exceeds socket buffer, forcing partial writes
    std::string payload(256 * 1024, 'X');
    std::string received;
    received.reserve(payload.size());

    TcpClient client(loop);
    client.on_connect([&] {
        client.send(payload);
    });
    client.on_data([&](const char* data, size_t len) {
        received.append(data, len);
        if (received.size() >= payload.size())
            loop.stop();
    });
    client.on_error([&](std::string_view) {
        loop.stop();
    });

    client.connect("127.0.0.1", port);

    auto timer = loop.add_timer(5000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(received.size() == payload.size());
    REQUIRE(received == payload);
}

TEST_CASE("TcpClient: error callback on connection refused", "[tcp_client]")
{
    EventLoop loop;

    // Listen and immediately close — port freed, nothing listens
    uint16_t port;
    {
        TcpListener tmp(0);
        port = tmp.local_port();
    }

    bool got_error = false;
    TcpClient client(loop);
    client.on_error([&](std::string_view) {
        got_error = true;
        loop.stop();
    });
    client.on_connect([&] {
        loop.stop();
    });

    client.connect("127.0.0.1", port);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_error);
    REQUIRE(client.state() == TcpClientState::Error);
}

TEST_CASE("TcpClient: idle timeout fires after inactivity", "[tcp_client]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    bool got_error = false;
    std::string error_msg;

    TcpClient client(loop);
    client.set_idle_timeout(100ms);
    client.on_connect([&] {
        // Connected — now idle timer starts. Don't send anything.
    });
    client.on_error([&](std::string_view msg) {
        got_error = true;
        error_msg = std::string(msg);
        loop.stop();
    });

    client.connect("127.0.0.1", port);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_error);
    REQUIRE(error_msg.find("idle") != std::string::npos);
}

TEST_CASE("TcpClient: connect timeout on unreachable address", "[.timeout]")
{
    // Hidden test — depends on network routing (192.0.2.1 may be routable on some systems)
    EventLoop loop;

    bool got_error = false;
    std::string error_msg;

    TcpClient client(loop);
    client.set_connect_timeout(500ms);
    client.on_error([&](std::string_view msg) {
        got_error = true;
        error_msg = std::string(msg);
        loop.stop();
    });

    // 192.0.2.1 is TEST-NET-1 (RFC 5737)
    client.connect("192.0.2.1", 9999);

    auto timer = loop.add_timer(3000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_error);
    REQUIRE(error_msg.find("timeout") != std::string::npos);
}

TEST_CASE("TcpClient: close triggers on_close callback", "[tcp_client]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    bool closed = false;
    TcpClient client(loop);
    client.on_connect([&] {
        client.close();
    });
    client.on_close([&] {
        closed = true;
        loop.stop();
    });
    client.on_error([&](std::string_view) {
        loop.stop();
    });

    client.connect("127.0.0.1", port);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(closed);
    REQUIRE(client.state() == TcpClientState::Closed);
}

TEST_CASE("TcpClient: multiple sends before connect completes", "[tcp_client]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    run_echo_server(loop, listener);

    std::string received;
    TcpClient client(loop);

    client.on_connect([&] {
        // data already queued
    });
    client.on_data([&](const char* data, size_t len) {
        received.append(data, len);
        if (received.size() >= 9)
            loop.stop();
    });
    client.on_error([&](std::string_view) {
        loop.stop();
    });

    client.connect("127.0.0.1", port);
    client.send("foo");
    client.send("bar");
    client.send("baz");

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(received == "foobarbaz");
}

// ─── TLS tests ───────────────────────────────────────────────────────────────

#ifdef WITH_SSL

TEST_CASE("TcpClient: TLS connect (smoke)", "[.ssl]")
{
    // Requires network access to example.com:443. Hidden by default.
    EventLoop loop;

    bool connected = false;
    TcpClient client(loop);
    client.enable_tls(false);
    client.set_connect_timeout(3000ms);

    client.on_connect([&] {
        connected = true;
        loop.stop();
    });
    client.on_error([&](std::string_view) {
        loop.stop();
    });

    client.connect("example.com", 443);

    auto timer = loop.add_timer(5000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(connected);
}

#endif // WITH_SSL
