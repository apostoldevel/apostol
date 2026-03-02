#include <catch2/catch_test_macros.hpp>

#include "apostol/websocket.hpp"
#include "apostol/ws_client.hpp"
#include "apostol/tcp.hpp"
#include "apostol/event_loop.hpp"

#include <chrono>
#include <cstdint>
#include <fmt/format.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

using namespace apostol;
using namespace std::chrono_literals;

// ─── ws_build_client_frame tests ────────────────────────────────────────────

TEST_CASE("ws_build_client_frame: MASK bit is set", "[ws_client]")
{
    auto frame = ws_build_client_frame(WS_OP_TEXT, "hello");
    REQUIRE(frame.size() >= 2);
    REQUIRE((static_cast<uint8_t>(frame[1]) & 0x80) != 0);
}

TEST_CASE("ws_build_client_frame: roundtrip via WsParser", "[ws_client]")
{
    std::string payload = "hello world";
    auto frame = ws_build_client_frame(WS_OP_TEXT, payload);

    std::string received;
    uint8_t received_opcode = 0;

    WsParser parser;
    parser.set_handler([&](uint8_t opcode, std::string data) {
        received_opcode = opcode;
        received = std::move(data);
    });

    REQUIRE(parser.feed(frame.data(), frame.size()));
    REQUIRE(received_opcode == WS_OP_TEXT);
    REQUIRE(received == payload);
}

TEST_CASE("ws_build_client_frame: binary opcode preserved", "[ws_client]")
{
    std::string data = "\x00\x01\x02\xFF";
    auto frame = ws_build_client_frame(WS_OP_BINARY, data);

    uint8_t got_opcode = 0;
    std::string got_data;

    WsParser parser;
    parser.set_handler([&](uint8_t op, std::string d) {
        got_opcode = op;
        got_data = std::move(d);
    });

    REQUIRE(parser.feed(frame.data(), frame.size()));
    REQUIRE(got_opcode == WS_OP_BINARY);
    REQUIRE(got_data == data);
}

TEST_CASE("ws_build_client_frame: empty payload", "[ws_client]")
{
    auto frame = ws_build_client_frame(WS_OP_PING, "");

    uint8_t got_opcode = 0;
    std::string got_data;

    WsParser parser;
    parser.set_handler([&](uint8_t op, std::string d) {
        got_opcode = op;
        got_data = std::move(d);
    });

    REQUIRE(parser.feed(frame.data(), frame.size()));
    REQUIRE(got_opcode == WS_OP_PING);
    REQUIRE(got_data.empty());
}

TEST_CASE("ws_build_client_frame: large payload (>125 bytes)", "[ws_client]")
{
    std::string payload(300, 'X');
    auto frame = ws_build_client_frame(WS_OP_TEXT, payload);

    std::string received;
    WsParser parser;
    parser.set_handler([&](uint8_t, std::string d) {
        received = std::move(d);
    });

    REQUIRE(parser.feed(frame.data(), frame.size()));
    REQUIRE(received == payload);
}

// ─── WsMessage tests ────────────────────────────────────────────────────────

TEST_CASE("WsMessage: JSON round-trip", "[ws_client]")
{
    WsMessage msg;
    msg.id = "abc123";
    msg.action = "greet";
    msg.payload = {{"name", "world"}};

    auto json_str = msg.to_json();
    auto parsed = WsMessage::from_json(json_str);

    REQUIRE(parsed.id == "abc123");
    REQUIRE(parsed.action == "greet");
    REQUIRE(parsed.payload["name"] == "world");
}

TEST_CASE("WsMessage: empty fields produce minimal JSON", "[ws_client]")
{
    WsMessage msg;
    auto json_str = msg.to_json();
    auto parsed = WsMessage::from_json(json_str);

    REQUIRE(parsed.id.empty());
    REQUIRE(parsed.action.empty());
    REQUIRE(parsed.payload.is_null());
}

TEST_CASE("WsMessage: from_json with invalid input returns empty", "[ws_client]")
{
    auto msg = WsMessage::from_json("not json");
    REQUIRE(msg.id.empty());
    REQUIRE(msg.action.empty());
}

TEST_CASE("WsMessage: payload can be array", "[ws_client]")
{
    WsMessage msg;
    msg.action = "batch";
    msg.payload = nlohmann::json::array({1, 2, 3});

    auto parsed = WsMessage::from_json(msg.to_json());
    REQUIRE(parsed.payload.is_array());
    REQUIRE(parsed.payload.size() == 3);
}

// ─── MockWsServer ───────────────────────────────────────────────────────────

struct MockWsServer
{
    EventLoop& loop;
    TcpListener listener{0};
    std::shared_ptr<TcpConnection> conn;
    WsParser parser;
    std::string http_buf;
    bool upgraded{false};
    bool auto_echo{false};

    std::vector<std::string> received_messages;
    std::vector<uint8_t>     received_opcodes;

    explicit MockWsServer(EventLoop& l) : loop(l)
    {
        parser.set_handler([this](uint8_t opcode, std::string payload) {
            received_opcodes.push_back(opcode);
            received_messages.push_back(payload);
            if (auto_echo && (opcode == WS_OP_TEXT || opcode == WS_OP_BINARY))
                send_frame(opcode, payload);
        });

        loop.add_io(listener.fd(), EPOLLIN, [this](uint32_t) {
            auto c = listener.accept();
            if (!c) return;
            conn = std::make_shared<TcpConnection>(std::move(*c));
            int fd = conn->fd();

            loop.add_io(fd, EPOLLIN, [this](uint32_t) {
                char buf[4096];
                ssize_t n = conn->read(buf, sizeof(buf));
                if (n <= 0) {
                    if (n == 0) loop.remove_io(conn->fd());
                    return;
                }

                if (!upgraded) {
                    http_buf.append(buf, static_cast<size_t>(n));
                    auto end = http_buf.find("\r\n\r\n");
                    if (end == std::string::npos) return;

                    // Extract Sec-WebSocket-Key
                    std::string key;
                    auto pos = http_buf.find("Sec-WebSocket-Key: ");
                    if (pos != std::string::npos) {
                        auto start = pos + 19;
                        auto eol = http_buf.find("\r\n", start);
                        key = http_buf.substr(start, eol - start);
                    }

                    auto accept = ws_accept_key(key);
                    std::string response = fmt::format(
                        "HTTP/1.1 101 Switching Protocols\r\n"
                        "Upgrade: websocket\r\n"
                        "Connection: Upgrade\r\n"
                        "Sec-WebSocket-Accept: {}\r\n"
                        "\r\n", accept);
                    conn->write(response.data(), response.size());
                    upgraded = true;

                    std::size_t ws_start = end + 4;
                    if (ws_start < http_buf.size())
                        parser.feed(http_buf.data() + ws_start,
                                    http_buf.size() - ws_start);
                    http_buf.clear();
                } else {
                    parser.feed(buf, static_cast<size_t>(n));
                }
            });
        });
    }

    uint16_t port() const { return listener.local_port(); }

    void send_frame(uint8_t opcode, std::string_view payload) {
        if (conn && upgraded) {
            auto frame = ws_build_frame(opcode, payload);
            conn->write(frame.data(), frame.size());
        }
    }

    void send_text(std::string_view text) { send_frame(WS_OP_TEXT, text); }

    void send_ping(std::string_view data = {}) { send_frame(WS_OP_PING, data); }

    void send_close(uint16_t code = 1000) {
        std::string payload;
        payload.push_back(static_cast<char>(code >> 8));
        payload.push_back(static_cast<char>(code & 0xFF));
        send_frame(WS_OP_CLOSE, payload);
    }
};

// ─── WsClient integration tests ─────────────────────────────────────────────

TEST_CASE("WsClient: construct and destruct", "[ws_client]")
{
    EventLoop loop;
    WsClient client(loop);

    REQUIRE(client.state() == WsClientState::Idle);
    REQUIRE_FALSE(client.connected());
}

TEST_CASE("WsClient: connect to mock server", "[ws_client]")
{
    EventLoop loop;
    MockWsServer server(loop);

    bool ws_connected = false;
    WsClient client(loop);
    client.set_ping_interval(std::chrono::seconds(0));
    client.on_connect([&] {
        ws_connected = true;
        loop.stop();
    });
    client.on_error([&](std::string_view) { loop.stop(); });

    auto url = fmt::format("ws://127.0.0.1:{}/ws", server.port());
    client.connect(url);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(ws_connected);
    REQUIRE(client.connected());
}

TEST_CASE("WsClient: send and receive text", "[ws_client]")
{
    EventLoop loop;
    MockWsServer server(loop);
    server.auto_echo = true;

    std::string received;
    WsClient client(loop);
    client.set_ping_interval(std::chrono::seconds(0));

    client.on_connect([&] {
        client.send_text("hello websocket");
    });
    client.on_message([&](uint8_t, std::string payload) {
        received = std::move(payload);
        loop.stop();
    });
    client.on_error([&](std::string_view) { loop.stop(); });

    auto url = fmt::format("ws://127.0.0.1:{}/", server.port());
    client.connect(url);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(received == "hello websocket");
}

TEST_CASE("WsClient: auto-pong on server ping", "[ws_client]")
{
    EventLoop loop;
    MockWsServer server(loop);

    bool connected = false;
    WsClient client(loop);
    client.set_ping_interval(std::chrono::seconds(0));

    client.on_connect([&] {
        connected = true;
        server.send_ping("ping-data");
    });
    client.on_error([&](std::string_view) { loop.stop(); });

    auto url = fmt::format("ws://127.0.0.1:{}/", server.port());
    client.connect(url);

    auto timer = loop.add_timer(500ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(connected);
    bool got_pong = false;
    for (size_t i = 0; i < server.received_opcodes.size(); ++i) {
        if (server.received_opcodes[i] == WS_OP_PONG) {
            got_pong = true;
            REQUIRE(server.received_messages[i] == "ping-data");
        }
    }
    REQUIRE(got_pong);
}

TEST_CASE("WsClient: close handshake", "[ws_client]")
{
    EventLoop loop;
    MockWsServer server(loop);

    uint16_t close_code = 0;
    WsClient client(loop);
    client.set_ping_interval(std::chrono::seconds(0));

    client.on_connect([&] {
        client.close(1000, "bye");
    });
    client.on_close([&](uint16_t code, std::string_view) {
        close_code = code;
        loop.stop();
    });
    client.on_error([&](std::string_view) { loop.stop(); });

    // Override parser to echo close frames
    server.parser.set_handler([&](uint8_t opcode, std::string payload) {
        server.received_opcodes.push_back(opcode);
        server.received_messages.push_back(payload);
        if (opcode == WS_OP_CLOSE) {
            server.send_close(1000);
        }
    });

    auto url = fmt::format("ws://127.0.0.1:{}/", server.port());
    client.connect(url);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(close_code == 1000);
    REQUIRE(client.state() == WsClientState::Closed);
}

TEST_CASE("WsClient: request/response correlation", "[ws_client]")
{
    EventLoop loop;
    MockWsServer server(loop);

    server.parser.set_handler([&](uint8_t opcode, std::string payload) {
        server.received_opcodes.push_back(opcode);
        server.received_messages.push_back(payload);
        if (opcode == WS_OP_TEXT) {
            auto msg = WsMessage::from_json(payload);
            WsMessage resp;
            resp.id = msg.id;
            resp.action = "reply";
            resp.payload = {{"echo", msg.payload}};
            server.send_text(resp.to_json());
        }
    });

    WsMessage response;
    bool got_response = false;

    WsClient client(loop);
    client.set_ping_interval(std::chrono::seconds(0));

    client.on_connect([&] {
        WsMessage req;
        req.action = "greet";
        req.payload = "hello";
        client.send(req, [&](const WsMessage& resp) {
            response = resp;
            got_response = true;
            loop.stop();
        });
    });
    client.on_error([&](std::string_view) { loop.stop(); });

    auto url = fmt::format("ws://127.0.0.1:{}/", server.port());
    client.connect(url);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_response);
    REQUIRE(response.action == "reply");
    REQUIRE(response.payload["echo"] == "hello");
}

TEST_CASE("WsClient: error on connection refused", "[ws_client]")
{
    EventLoop loop;

    uint16_t port;
    { TcpListener tmp(0); port = tmp.local_port(); }

    bool got_error = false;
    WsClient client(loop);
    client.on_error([&](std::string_view) {
        got_error = true;
        loop.stop();
    });

    auto url = fmt::format("ws://127.0.0.1:{}/", port);
    client.connect(url);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_error);
    REQUIRE(client.state() == WsClientState::Error);
}

TEST_CASE("WsClient: auto-reconnect on connection refused", "[ws_client]")
{
    EventLoop loop;

    uint16_t port;
    { TcpListener tmp(0); port = tmp.local_port(); }

    int error_count = 0;
    WsClient client(loop);
    client.auto_reconnect(true);
    client.set_reconnect_max_delay(std::chrono::seconds(1));
    client.on_error([&](std::string_view) {
        // Should NOT fire — auto_reconnect should handle it
        ++error_count;
        loop.stop();
    });

    auto url = fmt::format("ws://127.0.0.1:{}/", port);
    client.connect(url);

    // Let it try to reconnect a few times (each attempt fails immediately)
    auto timer = loop.add_timer(3000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    // With auto_reconnect, it should be in Reconnecting or Connecting, not Error
    REQUIRE(client.state() != WsClientState::Error);
    REQUIRE(error_count == 0);
}

TEST_CASE("WsClient: request/response timeout fires on_error", "[ws_client]")
{
    EventLoop loop;
    MockWsServer server(loop);

    // Server does NOT reply to messages (no echo)

    bool got_error = false;
    std::string error_msg;
    WsClient client(loop);
    client.set_ping_interval(std::chrono::seconds(0));

    client.on_connect([&] {
        WsMessage req;
        req.action = "test";
        req.payload = "data";
        client.send(req, [&](const WsMessage&) {
            // Should NOT be called
            loop.stop();
        }, 200ms);  // 200ms timeout
    });
    client.on_error([&](std::string_view msg) {
        got_error = true;
        error_msg = std::string(msg);
        loop.stop();
    });

    auto url = fmt::format("ws://127.0.0.1:{}/", server.port());
    client.connect(url);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_error);
    REQUIRE(error_msg.find("timeout") != std::string::npos);
}

TEST_CASE("WsClient: rejects invalid Sec-WebSocket-Accept", "[ws_client]")
{
    EventLoop loop;

    // Custom bad server that sends wrong Accept header
    TcpListener listener(0);
    std::shared_ptr<TcpConnection> srv_conn;

    loop.add_io(listener.fd(), EPOLLIN, [&](uint32_t) {
        auto c = listener.accept();
        if (!c) return;
        srv_conn = std::make_shared<TcpConnection>(std::move(*c));
        int fd = srv_conn->fd();

        loop.add_io(fd, EPOLLIN, [&](uint32_t) {
            char buf[4096];
            ssize_t n = srv_conn->read(buf, sizeof(buf));
            if (n <= 0) return;

            std::string req(buf, static_cast<size_t>(n));
            if (req.find("\r\n\r\n") == std::string::npos) return;

            // Send 101 with WRONG Accept value
            std::string response =
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: INVALID_VALUE_HERE\r\n"
                "\r\n";
            srv_conn->write(response.data(), response.size());
        });
    });

    bool got_error = false;
    std::string error_msg;
    WsClient client(loop);
    client.on_error([&](std::string_view msg) {
        got_error = true;
        error_msg = std::string(msg);
        loop.stop();
    });
    client.on_connect([&] {
        // Should NOT be called
        loop.stop();
    });

    auto url = fmt::format("ws://127.0.0.1:{}/", listener.local_port());
    client.connect(url);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(got_error);
    REQUIRE(error_msg.find("Sec-WebSocket-Accept") != std::string::npos);
}

TEST_CASE("WsClient: URL parsing (explicit port)", "[ws_client]")
{
    EventLoop loop;
    MockWsServer server(loop);

    bool connected = false;
    WsClient client(loop);
    client.set_ping_interval(std::chrono::seconds(0));
    client.on_connect([&] {
        connected = true;
        loop.stop();
    });
    client.on_error([&](std::string_view) { loop.stop(); });

    auto url = fmt::format("ws://127.0.0.1:{}/path/to/ws?token=abc", server.port());
    client.connect(url);

    auto timer = loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();
    loop.cancel_timer(timer);

    REQUIRE(connected);
}
