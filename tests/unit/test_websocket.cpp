#include <catch2/catch_test_macros.hpp>

#include "apostol/websocket.hpp"
#include "apostol/http.hpp"
#include "apostol/tcp.hpp"
#include "apostol/event_loop.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using namespace apostol;
using namespace std::chrono_literals;

// ─── Helpers ──────────────────────────────────────────────────────────────────

// Build a WebSocket frame as a CLIENT would send it (MASK=1, 4-byte mask key).
static std::string make_client_frame(uint8_t opcode,
                                     std::string_view payload,
                                     bool fin = true,
                                     uint8_t mask[4] = nullptr)
{
    static const uint8_t default_mask[4] = {0x37, 0xFA, 0x21, 0x3D};
    const uint8_t* m = mask ? mask : default_mask;

    std::string frame;
    frame.push_back(static_cast<char>((fin ? 0x80u : 0x00u) | opcode));

    auto len = payload.size();
    if (len < 126) {
        frame.push_back(static_cast<char>(0x80u | static_cast<uint8_t>(len)));
    } else if (len < 65536) {
        frame.push_back(static_cast<char>(0x80u | 126u));
        frame.push_back(static_cast<char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<char>(len & 0xFF));
    } else {
        frame.push_back(static_cast<char>(0x80u | 127u));
        for (int i = 7; i >= 0; --i)
            frame.push_back(static_cast<char>((len >> (8 * i)) & 0xFF));
    }

    // Mask key
    frame.append(reinterpret_cast<const char*>(m), 4);

    // Masked payload
    for (std::size_t i = 0; i < payload.size(); ++i)
        frame.push_back(static_cast<char>(
            static_cast<uint8_t>(payload[i]) ^ m[i % 4]));

    return frame;
}

// Connect a blocking client socket to 127.0.0.1:port
static int connect_to(uint16_t port)
{
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    REQUIRE(fd >= 0);
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    REQUIRE(::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    return fd;
}


// ─── ws_accept_key ────────────────────────────────────────────────────────────

TEST_CASE("ws_accept_key: RFC 6455 section 1.3 example vector", "[websocket]")
{
    // RFC 6455 §1.3: key = "dGhlIHNhbXBsZSBub25jZQ=="
    // Expected accept = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="
    REQUIRE(ws_accept_key("dGhlIHNhbXBsZSBub25jZQ==") == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

TEST_CASE("ws_accept_key: different keys produce different accepts", "[websocket]")
{
    auto a = ws_accept_key("dGhlIHNhbXBsZSBub25jZQ==");
    auto b = ws_accept_key("aGVsbG8gd29ybGQ=");
    REQUIRE(a != b);
}

// ─── is_ws_upgrade ────────────────────────────────────────────────────────────

static HttpRequest make_upgrade_request(std::string_view key = "dGhlIHNhbXBsZSBub25jZQ==",
                                        std::string_view protocol = "")
{
    HttpRequest req;
    req.method  = "GET";
    req.path    = "/ws";
    req.version = "HTTP/1.1";
    req.headers.emplace_back("Upgrade",               "websocket");
    req.headers.emplace_back("Connection",            "Upgrade");
    req.headers.emplace_back("Sec-WebSocket-Key",     std::string(key));
    req.headers.emplace_back("Sec-WebSocket-Version", "13");
    if (!protocol.empty())
        req.headers.emplace_back("Sec-WebSocket-Protocol", std::string(protocol));
    return req;
}

TEST_CASE("is_ws_upgrade: valid upgrade request returns true", "[websocket]")
{
    REQUIRE(is_ws_upgrade(make_upgrade_request()));
}

TEST_CASE("is_ws_upgrade: plain GET request returns false", "[websocket]")
{
    HttpRequest req;
    req.method = "GET";
    req.path   = "/index.html";
    REQUIRE_FALSE(is_ws_upgrade(req));
}

TEST_CASE("is_ws_upgrade: POST request returns false", "[websocket]")
{
    auto req = make_upgrade_request();
    req.method = "POST";
    REQUIRE_FALSE(is_ws_upgrade(req));
}

TEST_CASE("is_ws_upgrade: wrong Upgrade value returns false", "[websocket]")
{
    auto req = make_upgrade_request();
    // Replace Upgrade header with wrong value
    for (auto& [k, v] : req.headers)
        if (k == "Upgrade") v = "h2c";
    REQUIRE_FALSE(is_ws_upgrade(req));
}

TEST_CASE("is_ws_upgrade: missing Sec-WebSocket-Key returns false", "[websocket]")
{
    auto req = make_upgrade_request();
    // Remove Sec-WebSocket-Key
    std::erase_if(req.headers,
        [](const auto& p){ return p.first == "Sec-WebSocket-Key"; });
    REQUIRE_FALSE(is_ws_upgrade(req));
}

// ─── ws_build_frame ───────────────────────────────────────────────────────────

TEST_CASE("ws_build_frame: small text frame structure", "[websocket]")
{
    auto frame = ws_build_frame(WS_OP_TEXT, "Hi");

    // 2-byte header + 2-byte payload
    REQUIRE(frame.size() == 4);

    auto b0 = static_cast<uint8_t>(frame[0]);
    auto b1 = static_cast<uint8_t>(frame[1]);

    REQUIRE((b0 & 0x80) != 0);          // FIN set
    REQUIRE((b0 & 0x0F) == WS_OP_TEXT); // opcode TEXT
    REQUIRE((b1 & 0x80) == 0);          // no mask (server → client)
    REQUIRE((b1 & 0x7F) == 2);          // payload length = 2
    REQUIRE(frame[2] == 'H');
    REQUIRE(frame[3] == 'i');
}

TEST_CASE("ws_build_frame: FIN=false clears FIN bit", "[websocket]")
{
    auto frame = ws_build_frame(WS_OP_TEXT, "frag", /*fin=*/false);
    REQUIRE((static_cast<uint8_t>(frame[0]) & 0x80) == 0);
}

TEST_CASE("ws_build_frame: 16-bit extended length for payload >= 126", "[websocket]")
{
    std::string payload(200, 'x');
    auto frame = ws_build_frame(WS_OP_BINARY, payload);

    // header: byte0 + byte1(126) + 2-byte length + payload
    REQUIRE(frame.size() == 1 + 1 + 2 + 200);

    auto b1 = static_cast<uint8_t>(frame[1]);
    REQUIRE((b1 & 0x7F) == 126);

    uint16_t len = (static_cast<uint8_t>(frame[2]) << 8)
                 | static_cast<uint8_t>(frame[3]);
    REQUIRE(len == 200);
}

TEST_CASE("ws_build_frame: ping frame", "[websocket]")
{
    auto frame = ws_build_frame(WS_OP_PING, "");
    REQUIRE((static_cast<uint8_t>(frame[0]) & 0x0F) == WS_OP_PING);
    REQUIRE((static_cast<uint8_t>(frame[1]) & 0x7F) == 0);
}

// ─── WsParser ────────────────────────────────────────────────────────────────

TEST_CASE("WsParser: parse unmasked text frame (round-trip with ws_build_frame)", "[websocket]")
{
    WsParser parser;

    uint8_t got_opcode = 0xFF;
    std::string got_payload;

    parser.set_handler([&](uint8_t op, std::string payload) {
        got_opcode  = op;
        got_payload = std::move(payload);
    });

    auto frame = ws_build_frame(WS_OP_TEXT, "Hello");
    REQUIRE(parser.feed(frame.data(), frame.size()));

    REQUIRE(got_opcode  == WS_OP_TEXT);
    REQUIRE(got_payload == "Hello");
}

TEST_CASE("WsParser: parse masked text frame (client-style)", "[websocket]")
{
    WsParser parser;

    uint8_t got_opcode = 0xFF;
    std::string got_payload;
    parser.set_handler([&](uint8_t op, std::string p) {
        got_opcode  = op;
        got_payload = std::move(p);
    });

    auto frame = make_client_frame(WS_OP_TEXT, "World");
    REQUIRE(parser.feed(frame.data(), frame.size()));

    REQUIRE(got_opcode  == WS_OP_TEXT);
    REQUIRE(got_payload == "World");
}

TEST_CASE("WsParser: parse data fed byte by byte", "[websocket]")
{
    WsParser parser;
    int call_count = 0;
    std::string got;

    parser.set_handler([&](uint8_t, std::string p) {
        ++call_count;
        got = std::move(p);
    });

    auto frame = make_client_frame(WS_OP_TEXT, "byte");
    bool ok = true;
    for (char c : frame)
        ok &= parser.feed(&c, 1);

    REQUIRE(ok);
    REQUIRE(call_count == 1);
    REQUIRE(got == "byte");
}

TEST_CASE("WsParser: parse 16-bit extended length frame", "[websocket]")
{
    WsParser parser;
    std::string got;
    parser.set_handler([&](uint8_t, std::string p) { got = std::move(p); });

    std::string payload(200, 'Z');
    auto frame = make_client_frame(WS_OP_BINARY, payload);
    REQUIRE(parser.feed(frame.data(), frame.size()));
    REQUIRE(got == payload);
}

TEST_CASE("WsParser: ping frame is delivered with WS_OP_PING opcode", "[websocket]")
{
    WsParser parser;
    uint8_t got_op = 0;
    parser.set_handler([&](uint8_t op, std::string) { got_op = op; });

    auto frame = make_client_frame(WS_OP_PING, "ping-data");
    REQUIRE(parser.feed(frame.data(), frame.size()));
    REQUIRE(got_op == WS_OP_PING);
}

TEST_CASE("WsParser: fragmented message is reassembled", "[websocket]")
{
    WsParser parser;
    int call_count = 0;
    std::string got;
    parser.set_handler([&](uint8_t op, std::string p) {
        ++call_count;
        got = std::move(p);
    });

    // Fragment 1: FIN=false, opcode=TEXT
    auto frag1 = make_client_frame(WS_OP_TEXT, "Hel", /*fin=*/false);
    // Fragment 2: FIN=false, opcode=CONTINUATION
    auto frag2 = make_client_frame(WS_OP_CONTINUATION, "lo ", /*fin=*/false);
    // Fragment 3: FIN=true, opcode=CONTINUATION
    auto frag3 = make_client_frame(WS_OP_CONTINUATION, "World", /*fin=*/true);

    REQUIRE(parser.feed(frag1.data(), frag1.size()));
    REQUIRE(call_count == 0);  // not delivered yet

    REQUIRE(parser.feed(frag2.data(), frag2.size()));
    REQUIRE(call_count == 0);

    REQUIRE(parser.feed(frag3.data(), frag3.size()));
    REQUIRE(call_count == 1);
    REQUIRE(got == "Hello World");
}

TEST_CASE("WsParser: two consecutive frames", "[websocket]")
{
    WsParser parser;
    std::vector<std::string> messages;
    parser.set_handler([&](uint8_t, std::string p) { messages.push_back(std::move(p)); });

    auto f1 = make_client_frame(WS_OP_TEXT, "first");
    auto f2 = make_client_frame(WS_OP_TEXT, "second");

    std::string combined = f1 + f2;
    REQUIRE(parser.feed(combined.data(), combined.size()));
    REQUIRE(messages.size() == 2);
    REQUIRE(messages[0] == "first");
    REQUIRE(messages[1] == "second");
}

// ─── WsConnection ────────────────────────────────────────────────────────────

TEST_CASE("WsConnection: send_text then read from the other end", "[websocket]")
{
    // Create a connected socket pair
    int sv[2];
    REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    TcpConnection conn(sv[0], {}, 0);
    WsConnection ws(std::move(conn));

    ws.send_text("hello ws");

    // Read raw bytes from sv[1] and verify frame structure
    char buf[64] = {};
    ssize_t n = ::read(sv[1], buf, sizeof(buf));
    REQUIRE(n > 0);

    auto b0 = static_cast<uint8_t>(buf[0]);
    auto b1 = static_cast<uint8_t>(buf[1]);

    REQUIRE((b0 & 0x80) != 0);           // FIN
    REQUIRE((b0 & 0x0F) == WS_OP_TEXT);  // TEXT opcode
    REQUIRE((b1 & 0x80) == 0);           // no mask
    REQUIRE((b1 & 0x7F) == 8);           // "hello ws" = 8 bytes

    REQUIRE(std::string(buf + 2, 8) == "hello ws");

    ::close(sv[1]);
}

TEST_CASE("WsConnection: on_readable delivers text message", "[websocket]")
{
    int sv[2];
    REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    TcpConnection conn(sv[0], {}, 0);
    WsConnection ws(std::move(conn));

    // Write a client frame into sv[1]
    auto frame = make_client_frame(WS_OP_TEXT, "recv-test");
    (void)::write(sv[1], frame.data(), frame.size());
    ::shutdown(sv[1], SHUT_WR);  // signal EOF after frame

    std::string got;
    ws.on_readable([&](uint8_t, const std::string& p) { got = p; });

    REQUIRE(got == "recv-test");
    // keep may be false because we shut down write side → EOF
    ::close(sv[1]);
}

TEST_CASE("WsConnection: on_readable auto-responds to ping with pong", "[websocket]")
{
    int sv[2];
    REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    TcpConnection conn(sv[0], {}, 0);
    WsConnection ws(std::move(conn));

    auto ping = make_client_frame(WS_OP_PING, "ping!");
    (void)::write(sv[1], ping.data(), ping.size());
    ::shutdown(sv[1], SHUT_WR);

    ws.on_readable([](uint8_t, const std::string&){});

    // Read the pong frame from sv[1]
    struct timeval tv{0, 200'000};
    ::setsockopt(sv[1], SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    char buf[64] = {};
    ssize_t n = ::read(sv[1], buf, sizeof(buf));
    REQUIRE(n >= 2);

    REQUIRE((static_cast<uint8_t>(buf[0]) & 0x0F) == WS_OP_PONG);
    REQUIRE(std::string(buf + 2, static_cast<std::size_t>(n) - 2) == "ping!");

    ::close(sv[1]);
}

TEST_CASE("WsConnection: on_readable returns false on close frame", "[websocket]")
{
    int sv[2];
    REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    TcpConnection conn(sv[0], {}, 0);
    WsConnection ws(std::move(conn));

    // Close frame with code 1000
    std::string close_payload;
    close_payload.push_back(static_cast<char>(0x03));  // 1000 >> 8
    close_payload.push_back(static_cast<char>(0xE8));  // 1000 & 0xFF
    auto frame = make_client_frame(WS_OP_CLOSE, close_payload);
    (void)::write(sv[1], frame.data(), frame.size());
    ::shutdown(sv[1], SHUT_WR);

    bool keep = ws.on_readable([](uint8_t, const std::string&){});
    REQUIRE_FALSE(keep);

    ::close(sv[1]);
}

// ─── ws_upgrade integration ───────────────────────────────────────────────────

TEST_CASE("ws_upgrade: HTTP→WS handshake and message exchange via EventLoop", "[websocket]")
{
    EventLoop loop;
    TcpListener listener(0);
    uint16_t port = listener.local_port();

    std::string ws_message_received;
    std::string ws_reply_sent;
    int client_fd = -1;

    // Server side: accept, upgrade, echo text frames
    loop.add_io(listener.fd(), EPOLLIN, [&](uint32_t) {
        auto conn_opt = listener.accept();
        REQUIRE(conn_opt.has_value());

        auto http_conn = std::make_shared<HttpConnection>(std::move(*conn_opt));
        int fd = http_conn->fd();

        // Use shared ConnectionState for HTTP→WS transition
        struct State {
            enum class Mode { Http, WebSocket } mode{Mode::Http};
            std::shared_ptr<HttpConnection> http;
            std::shared_ptr<WsConnection>   ws;
        };
        auto state = std::make_shared<State>();
        state->http = http_conn;

        loop.add_io(fd, EPOLLIN | EPOLLRDHUP, [&loop, state, fd,
                                                &ws_message_received,
                                                &ws_reply_sent](uint32_t) {
            if (state->mode == State::Mode::Http) {
                bool keep = state->http->on_readable([&](const HttpRequest& req, HttpResponse& resp) {
                    // Try WebSocket upgrade
                    auto ws_opt = ws_upgrade(*state->http, req);
                    if (ws_opt) {
                        state->ws   = std::make_shared<WsConnection>(std::move(*ws_opt));
                        state->mode = State::Mode::WebSocket;
                    } else {
                        resp.set_status(200, "OK").set_body("ok");
                    }
                });
                if (!keep && state->mode == State::Mode::Http)
                    loop.remove_io(fd);
            } else {
                bool keep = state->ws->on_readable(
                    [&](uint8_t opcode, const std::string& payload) {
                        if (opcode == WS_OP_TEXT) {
                            ws_message_received = payload;
                            // Echo back
                            ws_reply_sent = payload + " -> pong";
                            state->ws->send_text(ws_reply_sent);
                        }
                    });
                if (!keep)
                    loop.remove_io(fd);
            }
        });
    });

    // Client: connect, do WS handshake, send text frame
    loop.add_timer(10ms, [&] {
        client_fd = connect_to(port);

        // Send HTTP upgrade request
        const std::string key = "dGhlIHNhbXBsZSBub25jZQ==";
        std::string req =
            "GET /ocpp/CP001 HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: " + key + "\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
        (void)::write(client_fd, req.data(), req.size());
    }, false);

    // After handshake: send a WS text frame
    loop.add_timer(80ms, [&] {
        if (client_fd < 0) return;
        // Read 101 response — single read with timeout (response fits in one segment)
        struct timeval tv{0, 100'000};
        ::setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        char resp_buf[512] = {};
        ssize_t resp_n = ::read(client_fd, resp_buf, sizeof(resp_buf));
        std::string resp(resp_buf, resp_n > 0 ? static_cast<std::size_t>(resp_n) : 0);
        REQUIRE(resp.find("101 Switching Protocols") != std::string::npos);
        REQUIRE(resp.find("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") != std::string::npos);

        // Send a text frame
        auto frame = make_client_frame(WS_OP_TEXT, "hello server");
        (void)::write(client_fd, frame.data(), frame.size());
    }, false);

    loop.add_timer(300ms, [&] { loop.stop(); }, false);
    loop.run();

    if (client_fd >= 0) {
        // Read echo frame
        char buf[256] = {};
        struct timeval tv{0, 200'000};
        ::setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ssize_t n = ::read(client_fd, buf, sizeof(buf));
        if (n > 2) {
            REQUIRE((static_cast<uint8_t>(buf[0]) & 0x0F) == WS_OP_TEXT);
            uint8_t len = static_cast<uint8_t>(buf[1]) & 0x7F;
            REQUIRE(std::string(buf + 2, len) == "hello server -> pong");
        }
        ::close(client_fd);
    }

    REQUIRE(ws_message_received == "hello server");
}
