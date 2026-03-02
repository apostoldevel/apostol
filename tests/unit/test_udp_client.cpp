#include <catch2/catch_test_macros.hpp>

#include "apostol/udp_client.hpp"
#include "apostol/event_loop.hpp"

#include <chrono>
#include <string>

using namespace apostol;
using namespace std::chrono_literals;

// ─── Smoke tests ─────────────────────────────────────────────────────────────

TEST_CASE("UdpClient: construct and destruct", "[udp_client]")
{
    EventLoop loop;
    UdpClient client(loop, 0);

    REQUIRE(client.fd() >= 0);
    REQUIRE(client.local_port() > 0);
    REQUIRE_FALSE(client.running());
}

TEST_CASE("UdpClient: start and stop EventLoop registration", "[udp_client]")
{
    EventLoop loop;
    UdpClient client(loop, 0);

    client.start();
    REQUIRE(client.running());

    client.stop();
    REQUIRE_FALSE(client.running());
}

// ─── Send/receive ────────────────────────────────────────────────────────────

TEST_CASE("UdpClient: send_to and receive via loopback", "[udp_client]")
{
    EventLoop loop;

    UdpClient sender(loop, 0);
    UdpClient receiver(loop, 0);

    std::string received;
    receiver.on_datagram([&](const UdpDatagram& dg) {
        received = dg.data;
        loop.stop();
    });
    receiver.start();

    // Send after a short delay to ensure receiver is registered
    loop.add_timer(10ms, [&] {
        sender.send_to("hello udp", "127.0.0.1", receiver.local_port());
    }, /*repeat=*/false);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(received == "hello udp");
}

TEST_CASE("UdpClient: reply to received datagram", "[udp_client]")
{
    EventLoop loop;

    UdpClient client_a(loop, 0);
    UdpClient client_b(loop, 0);

    std::string reply_data;

    // B echoes back anything received
    client_b.on_datagram([&](const UdpDatagram& dg) {
        client_b.reply(dg, "echo:" + dg.data);
    });
    client_b.start();

    // A receives the echo
    client_a.on_datagram([&](const UdpDatagram& dg) {
        reply_data = dg.data;
        loop.stop();
    });
    client_a.start();

    loop.add_timer(10ms, [&] {
        client_a.send_to("ping", "127.0.0.1", client_b.local_port());
    }, false);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(reply_data == "echo:ping");
}
