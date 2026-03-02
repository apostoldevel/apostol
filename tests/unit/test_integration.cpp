// tests/unit/test_integration.cpp
// Integration tests: Application + Module + PgPool + HTTP/WS
//
// Requires a local PostgreSQL server accessible via:
//   host=localhost port=5432 dbname=web user=http password=http sslmode=disable
//
// All tests use [integration] tag and can be run with:
//   ./apostol_tests "[integration]"
//
// LIFETIME NOTE: PgPool::~PgPool() calls loop_.remove_io(), so PgPool must be
// destroyed BEFORE EventLoop. In production, worker processes call std::exit()
// so destructors are not an issue. In tests, call stop_db() explicitly at the
// end of run_worker(), before EventLoop goes out of scope.

#include <catch2/catch_test_macros.hpp>

#include "apostol/application.hpp"
#include "apostol/event_loop.hpp"
#include "apostol/http.hpp"
#include "apostol/module.hpp"
#ifdef WITH_POSTGRESQL
#include "apostol/pg.hpp"
#endif
#include "apostol/websocket.hpp"

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

// ─── helpers ──────────────────────────────────────────────────────────────────

static const char* kConnInfo =
    "host=localhost port=5432 dbname=web user=http password=http sslmode=disable";

// Build a client (masked) WebSocket text frame.
static std::string make_ws_text_frame(std::string_view payload)
{
    constexpr uint8_t mask[4] = {0x37, 0xFA, 0x21, 0x3D};
    std::string frame;
    frame.push_back('\x81');  // FIN + TEXT opcode
    auto len = payload.size();
    if (len < 126) {
        frame.push_back(static_cast<char>(0x80u | static_cast<uint8_t>(len)));
    } else if (len < 65536) {
        frame.push_back(static_cast<char>(0x80u | 126u));
        frame.push_back(static_cast<char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<char>(len & 0xFF));
    }
    frame.append(reinterpret_cast<const char*>(mask), 4);
    for (std::size_t i = 0; i < payload.size(); ++i)
        frame.push_back(static_cast<char>(
            static_cast<uint8_t>(payload[i]) ^ mask[i % 4]));
    return frame;
}

// Register a non-blocking client socket in @p loop; calls @p on_connect once
// writable, then switches to EPOLLIN.
static void async_connect(EventLoop& loop, uint16_t port,
    std::function<void(EventLoop&, int fd)> on_connect)
{
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    REQUIRE(fd >= 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));  // EINPROGRESS ok

    loop.add_io(fd, EPOLLOUT,
        [&loop, fd, cb = std::move(on_connect)](uint32_t) mutable {
            loop.remove_io(fd);
            cb(loop, fd);
        });
}

#ifdef WITH_POSTGRESQL

// ─── Test fixtures ────────────────────────────────────────────────────────────

// A module that caches the result of a DB query run in heartbeat().
class DbCacheModule final : public Module
{
public:
    explicit DbCacheModule(PgPool& pool) : pool_(pool) {}

    std::string_view name()    const override { return "db_cache"; }
    bool             enabled() const override { return true; }

    bool execute(const HttpRequest& req, HttpResponse& resp) override
    {
        if (req.path != "/dbcache") return false;
        resp.set_status(200, "OK").set_body(cached_, "text/plain");
        return true;
    }

    void heartbeat(std::chrono::system_clock::time_point) override
    {
        if (heartbeat_fired_) return;
        heartbeat_fired_ = true;
        pool_.execute("SELECT 'integration_ok'::text AS v",
            [this](std::vector<PgResult> res) {
                if (!res.empty() && res[0].ok() && res[0].rows() > 0)
                    cached_ = res[0].value(0, 0);
                query_done_ = true;
            });
    }

    bool        heartbeat_fired_{false};
    bool        query_done_{false};
    std::string cached_;

private:
    PgPool& pool_;
};

// ─── Test 1: setup_db creates a working pool ──────────────────────────────────

TEST_CASE("setup_db creates working pool", "[integration]")
{
    class IntApp : public Application
    {
    public:
        IntApp() : Application("int_test") {}

        bool done_{false};
        bool ok_{false};

        void run_worker()
        {
            EventLoop loop;
            auto& pool = setup_db(loop, kConnInfo, /*min=*/1, /*max=*/2);

            // Fire query after connections are established.
            loop.add_timer(300ms, [this, &pool, &loop] {
                pool.execute("SELECT 42::int4 AS v",
                    [this, &loop](std::vector<PgResult> res) {
                        done_ = true;
                        ok_   = !res.empty() && res[0].ok()
                                && std::string(res[0].value(0, 0)) == "42";
                        loop.stop();
                    });
            }, /*repeat=*/false);

            loop.add_timer(4000ms, [&] { loop.stop(); }, false);
            loop.run();

            // IMPORTANT: destroy PgPool before EventLoop goes out of scope.
            // PgPool::~PgPool() calls loop_.remove_io() which requires a live loop.
            stop_db();
        }
    };

    IntApp app;
    app.run_worker();

    REQUIRE(app.done_);
    CHECK(app.ok_);
}

// ─── Test 2: db_pool — module constructor injection ───────────────────────────

TEST_CASE("db_pool access from module constructor injection", "[integration]")
{
    class IntApp : public Application
    {
    public:
        IntApp() : Application("int_test2") {}

        DbCacheModule* mod_{nullptr};
        uint16_t       port_{0};
        bool           http_cache_ok_{false};

        void run_worker()
        {
            EventLoop loop;
            auto& pool = setup_db(loop, kConnInfo, 1, 2);

            auto m = std::make_unique<DbCacheModule>(pool);
            mod_   = m.get();
            module_manager().add_module(std::move(m));

            start_http_server(loop, /*port=*/0);
            port_ = http_port();

            // Heartbeat fires at ~1 s (from start_http_server timer).
            // After 2 s the query should be done; send an HTTP request from
            // within the loop to verify the module serves the cached value.
            loop.add_timer(2000ms, [this, &loop] {
                if (!mod_->query_done_) {
                    // Query not done yet; stop and let assertions fail.
                    loop.stop();
                    return;
                }
                async_connect(loop, port_,
                    [this, &loop](EventLoop& ev_loop, int fd) {
                        std::string req =
                            "GET /dbcache HTTP/1.1\r\n"
                            "Host: localhost\r\n"
                            "Connection: close\r\n\r\n";
                        (void)::write(fd, req.data(), req.size());
                        ev_loop.add_io(fd, EPOLLIN,
                            [this, &ev_loop, fd](uint32_t) {
                                char buf[4096];
                                ssize_t n = ::read(fd, buf, sizeof(buf));
                                if (n > 0) {
                                    std::string_view r(buf,
                                        static_cast<std::size_t>(n));
                                    if (r.find("integration_ok") !=
                                            std::string_view::npos)
                                        http_cache_ok_ = true;
                                }
                                ev_loop.remove_io(fd);
                                ::close(fd);
                                ev_loop.stop();
                            });
                    });
            }, false);

            loop.add_timer(5000ms, [&] { loop.stop(); }, false);
            loop.run();

            // Destroy PgPool before EventLoop goes out of scope.
            stop_db();
        }
    };

    IntApp app;
    app.run_worker();

    REQUIRE(app.mod_->query_done_);
    CHECK(app.mod_->cached_ == "integration_ok");
    CHECK(app.http_cache_ok_);
}

#endif // WITH_POSTGRESQL

// ─── Test 3: WS upgrade via set_ws_handler ────────────────────────────────────

TEST_CASE("WS upgrade via set_ws_handler", "[integration]")
{
    class WsTestApp : public Application
    {
    public:
        WsTestApp() : Application("ws_test") {}

        uint16_t    port_{0};
        bool        ws_handler_called_{false};
        bool        echo_received_{false};
        std::string echo_payload_;

        void run_worker()
        {
            EventLoop loop;

            set_ws_handler(
                [this](EventLoop& ev_loop, WsConnection ws_conn, const HttpRequest&) {
                    ws_handler_called_ = true;
                    auto ws = std::make_shared<WsConnection>(std::move(ws_conn));
                    int  fd = ws->fd();
                    ev_loop.remove_io(fd);
                    ev_loop.add_io(fd, EPOLLIN,
                        [&ev_loop, ws, fd](uint32_t) {
                            bool ok = ws->on_readable(
                                [ws](uint8_t, const std::string& payload) {
                                    ws->send_text(payload);  // echo
                                },
                                [&ev_loop, fd] { ev_loop.remove_io(fd); });
                            if (!ok) ev_loop.remove_io(fd);
                        });
                });

            start_http_server(loop, /*port=*/0);
            port_ = http_port();

            // Connect a non-blocking WS client into the loop at 100 ms.
            loop.add_timer(100ms, [this, &loop] {
                async_connect(loop, port_,
                    [this, &loop](EventLoop& ev_loop, int fd) {
                        // Send WS upgrade.
                        std::string hs =
                            "GET /ws HTTP/1.1\r\n"
                            "Host: localhost\r\n"
                            "Upgrade: websocket\r\n"
                            "Connection: Upgrade\r\n"
                            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                            "Sec-WebSocket-Version: 13\r\n"
                            "\r\n";
                        (void)::write(fd, hs.data(), hs.size());

                        // Read the 101 response, then send a text frame.
                        // Use shared_ptr<bool> so the flag persists across
                        // callback copies (EventLoop copies cb before calling).
                        auto handshake_done = std::make_shared<bool>(false);
                        ev_loop.add_io(fd, EPOLLIN,
                            [this, &ev_loop, fd, handshake_done](uint32_t) {
                                char buf[4096];
                                ssize_t n = ::read(fd, buf, sizeof(buf));
                                if (n <= 0) {
                                    ev_loop.remove_io(fd);
                                    ::close(fd);
                                    ev_loop.stop();
                                    return;
                                }
                                std::string_view resp(buf,
                                    static_cast<std::size_t>(n));

                                if (!*handshake_done) {
                                    if (resp.find("101") !=
                                            std::string_view::npos) {
                                        *handshake_done = true;
                                        // Send echo frame.
                                        auto frame = make_ws_text_frame("hello");
                                        (void)::write(fd, frame.data(), frame.size());
                                    }
                                    return;
                                }

                                // Server echo: unmasked TEXT frame.
                                if (static_cast<uint8_t>(resp[0]) == 0x81
                                        && resp.size() >= 2) {
                                    uint8_t len =
                                        static_cast<uint8_t>(resp[1]) & 0x7F;
                                    if (resp.size() >= 2u + len) {
                                        echo_received_ = true;
                                        echo_payload_ =
                                            std::string(resp.data() + 2, len);
                                    }
                                }
                                ev_loop.remove_io(fd);
                                ::close(fd);
                                ev_loop.stop();
                            });
                    });
            }, false);

            loop.add_timer(3000ms, [&] { loop.stop(); }, false);
            loop.run();
        }
    };

    WsTestApp app;
    app.run_worker();

    CHECK(app.ws_handler_called_);
    CHECK(app.echo_received_);
    CHECK(app.echo_payload_ == "hello");
}

// ─── Test 4: WS + HTTP on same port ──────────────────────────────────────────

TEST_CASE("WS and HTTP served on same port", "[integration]")
{
    class DualApp : public Application
    {
    public:
        DualApp() : Application("dual_test") {}

        uint16_t port_{0};
        bool     ws_connected_{false};
        bool     http_ok_{false};

        class PingModule final : public Module
        {
        public:
            std::string_view name()    const override { return "ping"; }
            bool             enabled() const override { return true; }
            bool execute(const HttpRequest& req, HttpResponse& resp) override
            {
                if (req.path != "/ping") return false;
                resp.set_status(200, "OK").set_body("pong", "text/plain");
                return true;
            }
        };

        void run_worker()
        {
            EventLoop loop;

            module_manager().add_module(std::make_unique<PingModule>());

            set_ws_handler(
                [this](EventLoop& ev_loop, WsConnection ws_conn, const HttpRequest&) {
                    ws_connected_ = true;
                    auto ws = std::make_shared<WsConnection>(std::move(ws_conn));
                    int  fd = ws->fd();
                    ev_loop.remove_io(fd);
                    ev_loop.add_io(fd, EPOLLIN,
                        [&ev_loop, ws, fd](uint32_t) {
                            bool ok = ws->on_readable(
                                [ws](uint8_t, const std::string& p) {
                                    ws->send_text(p);
                                },
                                [&ev_loop, fd] { ev_loop.remove_io(fd); });
                            if (!ok) ev_loop.remove_io(fd);
                        });
                });

            start_http_server(loop, /*port=*/0);
            port_ = http_port();

            // HTTP client at 100 ms.
            loop.add_timer(100ms, [this, &loop] {
                async_connect(loop, port_,
                    [this, &loop](EventLoop& ev_loop, int fd) {
                        std::string req =
                            "GET /ping HTTP/1.1\r\n"
                            "Host: localhost\r\n"
                            "Connection: close\r\n\r\n";
                        (void)::write(fd, req.data(), req.size());
                        ev_loop.add_io(fd, EPOLLIN,
                            [this, &ev_loop, fd](uint32_t) {
                                char buf[2048];
                                ssize_t n = ::read(fd, buf, sizeof(buf));
                                if (n > 0) {
                                    std::string_view r(buf,
                                        static_cast<std::size_t>(n));
                                    if (r.find("pong") != std::string_view::npos)
                                        http_ok_ = true;
                                }
                                ev_loop.remove_io(fd);
                                ::close(fd);
                            });
                    });
            }, false);

            // WS client at 200 ms.
            loop.add_timer(200ms, [this, &loop] {
                async_connect(loop, port_,
                    [this, &loop](EventLoop& ev_loop, int fd) {
                        std::string hs =
                            "GET /ws HTTP/1.1\r\n"
                            "Host: localhost\r\n"
                            "Upgrade: websocket\r\n"
                            "Connection: Upgrade\r\n"
                            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                            "Sec-WebSocket-Version: 13\r\n"
                            "\r\n";
                        (void)::write(fd, hs.data(), hs.size());
                        ev_loop.add_io(fd, EPOLLIN,
                            [&ev_loop, fd](uint32_t) {
                                char buf[2048];
                                ssize_t n = ::read(fd, buf, sizeof(buf));
                                (void)n;
                                // Got 101 or data — just disconnect.
                                ev_loop.remove_io(fd);
                                ::close(fd);
                            });
                    });
            }, false);

            loop.add_timer(1500ms, [&] { loop.stop(); }, false);
            loop.run();
        }
    };

    DualApp app;
    app.run_worker();

    CHECK(app.ws_connected_);
    CHECK(app.http_ok_);
}

#ifdef WITH_POSTGRESQL

// ─── Test 5: PgPool LISTEN + HTTP serving ─────────────────────────────────────

TEST_CASE("PgPool listen and HTTP serving work in parallel", "[integration]")
{
    class ListenApp : public Application
    {
    public:
        ListenApp() : Application("listen_test") {}

        uint16_t port_{0};
        bool     http_ok_{false};
        bool     pool_started_{false};

        class OkModule final : public Module
        {
        public:
            std::string_view name()    const override { return "ok"; }
            bool             enabled() const override { return true; }
            bool execute(const HttpRequest& req, HttpResponse& resp) override
            {
                if (req.path != "/ok") return false;
                resp.set_status(200, "OK").set_body("ok", "text/plain");
                return true;
            }
        };

        void run_worker()
        {
            EventLoop loop;
            auto& pool = setup_db(loop, kConnInfo, 1, 2);
            pool_started_ = true;

            // Subscribe to a channel (pool acts as listener; verifies no crash).
            pool.listen("integration_test_channel",
                [](std::string_view, std::string_view) {});

            module_manager().add_module(std::make_unique<OkModule>());

            start_http_server(loop, /*port=*/0);
            port_ = http_port();

            // HTTP client after connections establish (~500 ms).
            loop.add_timer(500ms, [this, &loop] {
                async_connect(loop, port_,
                    [this, &loop](EventLoop& ev_loop, int fd) {
                        std::string req =
                            "GET /ok HTTP/1.1\r\n"
                            "Host: localhost\r\n"
                            "Connection: close\r\n\r\n";
                        (void)::write(fd, req.data(), req.size());
                        ev_loop.add_io(fd, EPOLLIN,
                            [this, &ev_loop, fd](uint32_t) {
                                char buf[2048];
                                ssize_t n = ::read(fd, buf, sizeof(buf));
                                if (n > 0) {
                                    std::string_view r(buf,
                                        static_cast<std::size_t>(n));
                                    if (r.find("200") != std::string_view::npos)
                                        http_ok_ = true;
                                }
                                ev_loop.remove_io(fd);
                                ::close(fd);
                                ev_loop.stop();
                            });
                    });
            }, false);

            loop.add_timer(3000ms, [&] { loop.stop(); }, false);
            loop.run();

            // Destroy PgPool before EventLoop goes out of scope.
            stop_db();
        }
    };

    ListenApp app;
    app.run_worker();

    CHECK(app.pool_started_);
    CHECK(app.http_ok_);
}

#endif // WITH_POSTGRESQL

// ─── Test 6: ProcessRole — new enum values and role_name() ───────────────────

TEST_CASE("ProcessRole: all roles have valid role_name()", "[process]")
{
    // Existing roles still work
    CHECK(role_name(ProcessRole::master)    == "master");
    CHECK(role_name(ProcessRole::worker)    == "worker");
    CHECK(role_name(ProcessRole::helper)    == "helper");

    // New roles added in v2 process model refactoring
    CHECK(role_name(ProcessRole::single)    == "single");
    CHECK(role_name(ProcessRole::custom)    == "custom");
    CHECK(role_name(ProcessRole::signaller) == "signaller");
}

// ─── Test 7: Module lifecycle — on_start / on_stop ───────────────────────────
//
// Verifies that ModuleManager calls Module::on_start() / on_stop() in
// the correct sequence (same contract as single_run / worker_run).

TEST_CASE("ModuleManager: on_start and on_stop are called for enabled modules", "[process]")
{
    class LifecycleModule final : public Module
    {
    public:
        std::vector<std::string> events;

        std::string_view name()    const override { return "lifecycle"; }
        bool             enabled() const override { return true; }

        bool execute(const HttpRequest&, HttpResponse&) override { return false; }

        void on_start() override { events.push_back("start"); }
        void on_stop()  override { events.push_back("stop"); }
    };

    ModuleManager mm;
    auto* m = new LifecycleModule;
    mm.add_module(std::unique_ptr<Module>(m));

    REQUIRE(m->events.empty());

    mm.on_start();
    REQUIRE(m->events.size() == 1);
    CHECK(m->events[0] == "start");

    mm.on_stop();
    REQUIRE(m->events.size() == 2);
    CHECK(m->events[1] == "stop");
}

// ─── Test 8: Application single mode — on_worker_start + module lifecycle ─────
//
// Simulates single_run() behaviour without fork by using the same pattern
// as integration tests (direct EventLoop, bypassing run()).

TEST_CASE("Application single mode: module lifecycle via on_worker_start", "[process]")
{
    class SingleModeApp : public Application
    {
    public:
        SingleModeApp() : Application("single_test") {}

        bool start_called{false};
        bool stop_called{false};

        // Lifecycle-tracking module declared inside the app
        class TrackModule final : public Module
        {
        public:
            bool& start_ref;
            bool& stop_ref;
            TrackModule(bool& s, bool& e) : start_ref(s), stop_ref(e) {}

            std::string_view name()    const override { return "track"; }
            bool             enabled() const override { return true; }
            bool execute(const HttpRequest&, HttpResponse&) override { return false; }
            void on_start() override { start_ref = true; }
            void on_stop()  override { stop_ref  = true; }
        };

        void run_single_test()
        {
            EventLoop loop;

            // Simulate on_worker_start: register module
            module_manager().add_module(
                std::make_unique<TrackModule>(start_called, stop_called));

            // Call module lifecycle explicitly (mirrors single_run / worker_run)
            module_manager().on_start();

            // Single tick of the loop, then stop
            loop.add_timer(std::chrono::milliseconds(10), [&] { loop.stop(); }, false);
            loop.run();

            module_manager().on_stop();
        }
    };

    SingleModeApp app;
    app.run_single_test();

    CHECK(app.start_called);
    CHECK(app.stop_called);
}
