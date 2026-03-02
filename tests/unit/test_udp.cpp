#include <catch2/catch_test_macros.hpp>

#include "apostol/udp.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

using namespace apostol;

// ─── UdpSocket creation ─────────────────────────────────────────────────────

TEST_CASE("UdpSocket: fd is valid and local_port returns non-zero", "[udp]")
{
    UdpSocket sock(0); // port=0 → OS picks a free port
    REQUIRE(sock.fd() >= 0);
    REQUIRE(sock.local_port() > 0);
}

TEST_CASE("UdpSocket: bind to specific port", "[udp]")
{
    UdpSocket sock(0);
    auto port = sock.local_port();
    REQUIRE(port > 0);

    // With SO_REUSEADDR, second bind to same UDP port succeeds (kernel allows it).
    // Verify both sockets get the same port.
    UdpSocket sock2(port);
    REQUIRE(sock2.local_port() == port);
}

// ─── Send / recv ─────────────────────────────────────────────────────────────

TEST_CASE("UdpSocket: loopback send and recv", "[udp]")
{
    UdpSocket server(0);
    auto port = server.local_port();

    // Create a client socket to send from
    int client_fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    REQUIRE(client_fd >= 0);

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);

    const char* msg = "hello UDP";
    auto sent = ::sendto(client_fd, msg, std::strlen(msg), 0,
                         reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
    REQUIRE(sent == static_cast<ssize_t>(std::strlen(msg)));

    auto dgram = server.recv();
    REQUIRE(dgram.has_value());
    REQUIRE(dgram->data == "hello UDP");
    REQUIRE(dgram->peer_ip() == "127.0.0.1");
    REQUIRE(dgram->peer_port() > 0);

    ::close(client_fd);
}

// ─── reply ───────────────────────────────────────────────────────────────────

TEST_CASE("UdpSocket: reply sends data back to sender", "[udp]")
{
    UdpSocket server(0);
    auto port = server.local_port();

    // Client socket (bound so we can recv reply)
    int client_fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    REQUIRE(client_fd >= 0);

    sockaddr_in client_addr{};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port   = 0;
    ::inet_pton(AF_INET, "127.0.0.1", &client_addr.sin_addr);
    REQUIRE(::bind(client_fd, reinterpret_cast<sockaddr*>(&client_addr),
                   sizeof(client_addr)) == 0);

    // Send to server
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);

    const char* msg = "ping";
    ::sendto(client_fd, msg, 4, 0, reinterpret_cast<sockaddr*>(&dest), sizeof(dest));

    auto dgram = server.recv();
    REQUIRE(dgram.has_value());

    // Reply
    auto r = server.reply(*dgram, "pong");
    REQUIRE(r == 4);

    // Receive reply on client
    char buf[16]{};
    auto n = ::recv(client_fd, buf, sizeof(buf), 0);
    REQUIRE(n == 4);
    REQUIRE(std::string_view(buf, 4) == "pong");

    ::close(client_fd);
}

// ─── recv on empty socket ────────────────────────────────────────────────────

TEST_CASE("UdpSocket: recv on empty socket returns nullopt", "[udp]")
{
    UdpSocket sock(0);
    auto dgram = sock.recv();
    REQUIRE_FALSE(dgram.has_value());
}

// ─── Move semantics ─────────────────────────────────────────────────────────

TEST_CASE("UdpSocket: move constructor transfers ownership", "[udp]")
{
    UdpSocket a(0);
    int orig_fd = a.fd();
    auto orig_port = a.local_port();
    REQUIRE(orig_fd >= 0);

    UdpSocket b(std::move(a));
    REQUIRE(b.fd() == orig_fd);
    REQUIRE(b.local_port() == orig_port);
    REQUIRE(a.fd() == -1); // NOLINT(bugprone-use-after-move)
}

TEST_CASE("UdpSocket: move assignment transfers ownership", "[udp]")
{
    UdpSocket a(0);
    UdpSocket b(0);
    int fd_a = a.fd();

    b = std::move(a);
    REQUIRE(b.fd() == fd_a);
    REQUIRE(a.fd() == -1); // NOLINT(bugprone-use-after-move)
}

// ─── UdpSocket with UdpSocket send/recv ─────────────────────────────────────

TEST_CASE("UdpSocket: send between two UdpSockets", "[udp]")
{
    UdpSocket server(0);
    UdpSocket client(0);

    // Build destination address for server
    sockaddr_storage dest{};
    auto* sa = reinterpret_cast<sockaddr_in*>(&dest);
    sa->sin_family = AF_INET;
    sa->sin_port   = htons(server.local_port());
    ::inet_pton(AF_INET, "127.0.0.1", &sa->sin_addr);

    std::string_view msg = "via UdpSocket";
    auto sent = client.send(msg.data(), msg.size(), dest, sizeof(sockaddr_in));
    REQUIRE(sent == static_cast<ssize_t>(msg.size()));

    auto dgram = server.recv();
    REQUIRE(dgram.has_value());
    REQUIRE(dgram->data == "via UdpSocket");
    REQUIRE(dgram->peer_port() == client.local_port());
}

// ─── Binary data ─────────────────────────────────────────────────────────────

TEST_CASE("UdpSocket: handles binary data with null bytes", "[udp]")
{
    UdpSocket server(0);

    int client_fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    REQUIRE(client_fd >= 0);

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(server.local_port());
    ::inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);

    // Binary payload with embedded nulls
    const char binary[] = {'\x01', '\x00', '\x02', '\x00', '\x03'};
    ::sendto(client_fd, binary, sizeof(binary), 0,
             reinterpret_cast<sockaddr*>(&dest), sizeof(dest));

    auto dgram = server.recv();
    REQUIRE(dgram.has_value());
    REQUIRE(dgram->data.size() == 5);
    REQUIRE(dgram->data[0] == '\x01');
    REQUIRE(dgram->data[1] == '\x00');
    REQUIRE(dgram->data[2] == '\x02');
    REQUIRE(dgram->data[3] == '\x00');
    REQUIRE(dgram->data[4] == '\x03');

    ::close(client_fd);
}
