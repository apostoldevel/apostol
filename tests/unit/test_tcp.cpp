#include <catch2/catch_test_macros.hpp>

#include "apostol/tcp.hpp"
#include "apostol/event_loop.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <memory>
#include <string_view>

using namespace apostol;
using namespace std::chrono_literals;

// ─── Helper ───────────────────────────────────────────────────────────────────

// Blocking connect to 127.0.0.1:port — returns the connected fd
static int connect_to(uint16_t port)
{
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    REQUIRE(fd >= 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    int rc = ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    REQUIRE(rc == 0);
    return fd;
}

// ─── TcpListener tests ────────────────────────────────────────────────────────

TEST_CASE("TcpListener: fd is valid and local_port returns a non-zero port", "[tcp]")
{
    TcpListener listener(0); // port=0 → OS picks a free port
    REQUIRE(listener.fd() >= 0);
    REQUIRE(listener.local_port() > 0);
}

TEST_CASE("TcpListener: accept returns connection after client connects", "[tcp]")
{
    TcpListener listener(0);
    int client_fd = connect_to(listener.local_port());

    auto conn = listener.accept();
    REQUIRE(conn.has_value());
    REQUIRE(conn->fd() >= 0);

    ::close(client_fd);
}

TEST_CASE("TcpListener: accept returns nullopt when no pending connection", "[tcp]")
{
    TcpListener listener(0);
    // Nothing connected — non-blocking accept should return EAGAIN → nullopt
    auto conn = listener.accept();
    REQUIRE_FALSE(conn.has_value());
}

// ─── TcpConnection tests ──────────────────────────────────────────────────────

TEST_CASE("TcpConnection: read returns data sent by peer", "[tcp]")
{
    TcpListener listener(0);
    int client_fd = connect_to(listener.local_port());

    auto conn = listener.accept();
    REQUIRE(conn.has_value());

    (void)::write(client_fd, "hello", 5);

    char buf[16]{};
    ssize_t n = -1;
    // Non-blocking: retry a few times to let loopback deliver
    for (int i = 0; i < 20 && n <= 0; ++i) {
        ::usleep(1'000);
        n = conn->read(buf, sizeof(buf));
    }

    REQUIRE(n == 5);
    REQUIRE(std::string_view(buf, static_cast<size_t>(n)) == "hello");

    ::close(client_fd);
}

TEST_CASE("TcpConnection: write sends data readable by peer", "[tcp]")
{
    TcpListener listener(0);
    int client_fd = connect_to(listener.local_port());

    auto conn = listener.accept();
    REQUIRE(conn.has_value());

    ssize_t sent = conn->write("world", 5);
    REQUIRE(sent == 5);

    char buf[16]{};
    ssize_t n = ::read(client_fd, buf, sizeof(buf));
    REQUIRE(n == 5);
    REQUIRE(std::string_view(buf, static_cast<size_t>(n)) == "world");

    ::close(client_fd);
}

TEST_CASE("TcpConnection: read returns 0 on peer close (EOF)", "[tcp]")
{
    TcpListener listener(0);
    int client_fd = connect_to(listener.local_port());

    auto conn = listener.accept();
    REQUIRE(conn.has_value());

    ::close(client_fd);
    ::usleep(5'000); // let FIN propagate

    char buf[8]{};
    ssize_t n = conn->read(buf, sizeof(buf));
    REQUIRE(n == 0); // EOF
}

TEST_CASE("TcpConnection: peer_address and peer_port are populated", "[tcp]")
{
    TcpListener listener(0);
    int client_fd = connect_to(listener.local_port());

    auto conn = listener.accept();
    REQUIRE(conn.has_value());

    REQUIRE_FALSE(conn->peer_address().empty());
    REQUIRE(conn->peer_port() > 0);

    ::close(client_fd);
}

TEST_CASE("TcpConnection: read returns -1 on EAGAIN (no data)", "[tcp]")
{
    TcpListener listener(0);
    int client_fd = connect_to(listener.local_port());

    auto conn = listener.accept();
    REQUIRE(conn.has_value());

    // Nothing written — non-blocking read should give EAGAIN → -1
    char buf[8]{};
    ssize_t n = conn->read(buf, sizeof(buf));
    REQUIRE(n == -1);

    ::close(client_fd);
}

// ─── EventLoop integration ────────────────────────────────────────────────────

TEST_CASE("TcpListener + EventLoop: EPOLLIN fires when client connects", "[tcp]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    bool accepted = false;
    int client_fd = -1;

    loop.add_io(listener.fd(), EPOLLIN, [&](uint32_t) {
        auto conn = listener.accept();
        REQUIRE(conn.has_value());
        accepted = true;
        loop.stop();
    });

    loop.add_timer(10ms, [&] {
        client_fd = connect_to(port);
    }, /*repeat=*/false);

    loop.run();

    REQUIRE(accepted);
    if (client_fd >= 0) ::close(client_fd);
}

TEST_CASE("TcpConnection + EventLoop: EPOLLIN fires when peer sends data", "[tcp]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    std::string received;
    int client_fd = -1;

    loop.add_io(listener.fd(), EPOLLIN, [&](uint32_t) {
        auto conn_opt = listener.accept();
        REQUIRE(conn_opt.has_value());

        // Heap-allocate so the lambda can share ownership
        auto conn = std::make_shared<TcpConnection>(std::move(*conn_opt));
        int conn_fd = conn->fd();

        loop.add_io(conn_fd, EPOLLIN | EPOLLRDHUP, [&loop, &received, conn](uint32_t) {
            char buf[64]{};
            ssize_t n = conn->read(buf, sizeof(buf));
            if (n > 0)
                received.append(buf, static_cast<size_t>(n));
            loop.stop();
        });
    });

    loop.add_timer(10ms, [&] {
        client_fd = connect_to(port);
        (void)::write(client_fd, "ping", 4);
    }, /*repeat=*/false);

    loop.run();

    REQUIRE(received == "ping");
    if (client_fd >= 0) ::close(client_fd);
}
