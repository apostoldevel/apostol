// tests/unit/test_pg.cpp
// TDD tests for PostgreSQL client layer.
//
// Requires a local PostgreSQL server accessible via:
//   host=localhost port=5432 dbname=postgres user=postgres
//
// Tests are split into:
//   [pg_result]      — PgResult accessors via blocking PQexec
//   [pg_connection]  — async PgConnection lifecycle
//   [pg_pool]        — PgPool with EventLoop integration

#include <catch2/catch_test_macros.hpp>

#include "apostol/event_loop.hpp"
#include "apostol/pg.hpp"

#include <libpq-fe.h>

#include <chrono>
#include <string>
#include <vector>

using namespace apostol;
using namespace std::chrono_literals;

// ─── helpers ──────────────────────────────────────────────────────────────────

static const char* kConnInfo = "host=localhost port=5432 dbname=web user=http password=http sslmode=disable";

/// Run a synchronous query (for PgResult tests only – does NOT go through PgPool).
static PgResult blocking_exec(const char* sql)
{
    PGconn* c = PQconnectdb(kConnInfo);
    REQUIRE(PQstatus(c) == CONNECTION_OK);
    PGresult* r = PQexec(c, sql);
    PQfinish(c);
    return PgResult(r);   // PgResult takes ownership
}

// ─────────────────────────────────────────────────────────────────────────────
// PgResult
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("PgResult — command ok (no rows)", "[pg_result]")
{
    auto res = blocking_exec("SELECT 1");
    REQUIRE(res.ok());
    REQUIRE(res.rows() == 1);
    REQUIRE(res.columns() == 1);
}

TEST_CASE("PgResult — column metadata", "[pg_result]")
{
    auto res = blocking_exec("SELECT 42::int4 AS answer, 'hello'::text AS greeting");
    REQUIRE(res.ok());
    REQUIRE(res.rows() == 1);
    REQUIRE(res.columns() == 2);

    // column names
    CHECK(std::string(res.column_name(0)) == "answer");
    CHECK(std::string(res.column_name(1)) == "greeting");

    // column index lookup
    CHECK(res.column_index("answer")   == 0);
    CHECK(res.column_index("greeting") == 1);
    CHECK(res.column_index("nonexist") == -1);
}

TEST_CASE("PgResult — value access", "[pg_result]")
{
    auto res = blocking_exec("SELECT 7::int4 AS n, 'world'::text AS s");
    REQUIRE(res.ok());

    CHECK(std::string(res.value(0, 0)) == "7");
    CHECK(std::string(res.value(0, 1)) == "world");

    CHECK(res.is_null(0, 0) == false);
    CHECK(res.is_null(0, 1) == false);
}

TEST_CASE("PgResult — null value", "[pg_result]")
{
    auto res = blocking_exec("SELECT NULL::text AS n");
    REQUIRE(res.ok());
    CHECK(res.is_null(0, 0) == true);
    // value() returns empty string for null
    CHECK(std::string(res.value(0, 0)) == "");
}

TEST_CASE("PgResult — multiple rows", "[pg_result]")
{
    auto res = blocking_exec("SELECT generate_series(1,5) AS n");
    REQUIRE(res.ok());
    REQUIRE(res.rows() == 5);
    for (int i = 0; i < 5; ++i)
        CHECK(std::string(res.value(i, 0)) == std::to_string(i + 1));
}

TEST_CASE("PgResult — error result is not ok", "[pg_result]")
{
    auto res = blocking_exec("SELECT * FROM nonexistent_table_xyz");
    CHECK_FALSE(res.ok());
    // error_message should be non-empty
    CHECK(std::string(res.error_message()).size() > 0);
}

TEST_CASE("PgResult — move semantics", "[pg_result]")
{
    auto res1 = blocking_exec("SELECT 1");
    REQUIRE(res1.ok());

    auto res2 = std::move(res1);
    REQUIRE(res2.ok());
    CHECK(res2.rows() == 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// PgConnection — async connect + query
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("PgConnection — async connect succeeds", "[pg_connection]")
{
    EventLoop loop;
    bool connected = false;
    bool done      = false;

    PgConnection conn(kConnInfo);
    REQUIRE(conn.connect_start());
    // After connect_start() the fd is known
    int fd = conn.fd();
    REQUIRE(fd >= 0);

    loop.add_io(fd, EPOLLIN | EPOLLOUT, [&](uint32_t /*events*/) {
        auto poll_status = conn.connect_poll();
        if (poll_status == PGRES_POLLING_OK) {
            connected = true;
            loop.stop();
        } else if (poll_status == PGRES_POLLING_FAILED) {
            done = true;
            loop.stop();
        }
    });

    loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();

    CHECK(connected);
    CHECK(conn.connected());
    CHECK(conn.state() == PgConnState::Ready);
}

TEST_CASE("PgConnection — send query and collect results", "[pg_connection]")
{
    EventLoop loop;
    std::vector<PgResult> results;
    bool query_done = false;

    PgConnection conn(kConnInfo);
    REQUIRE(conn.connect_start());

    // Phase 1: connect
    bool phase_connected = false;

    int fd = conn.fd();
    loop.add_io(fd, EPOLLIN | EPOLLOUT, [&](uint32_t /*events*/) {
        if (!phase_connected) {
            auto ps = conn.connect_poll();
            if (ps == PGRES_POLLING_OK) {
                phase_connected = true;
                // Phase 2: send query
                REQUIRE(conn.send_query("SELECT 1 AS v"));
                conn.set_state(PgConnState::Busy);
                // Now wait for readable to collect results
                loop.modify_io(fd, EPOLLIN);
            } else if (ps == PGRES_POLLING_FAILED) {
                loop.stop();
            }
        } else {
            // Phase 2: collect results
            results = conn.collect_results();
            query_done = true;
            loop.stop();
        }
    });

    loop.add_timer(3000ms, [&] { loop.stop(); }, false);
    loop.run();

    REQUIRE(query_done);
    REQUIRE(results.size() == 1);
    CHECK(results[0].ok());
    CHECK(results[0].rows() == 1);
    CHECK(std::string(results[0].value(0, 0)) == "1");
}

TEST_CASE("PgConnection — connection refused gives error", "[pg_connection]")
{
    PgConnection conn("host=localhost port=1 dbname=postgres user=postgres connect_timeout=1");
    // connect_start may fail immediately or return false
    bool started = conn.connect_start();
    if (started) {
        // poll once — should quickly fail
        auto ps = conn.connect_poll();
        CHECK(ps == PGRES_POLLING_FAILED);
    } else {
        CHECK(conn.state() == PgConnState::Error);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PgPool
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("PgPool — start opens min connections", "[pg_pool]")
{
    EventLoop loop;
    PgPool pool(loop, kConnInfo, /*min=*/2, /*max=*/4);
    pool.start();

    // Give connections time to establish
    int ticks = 0;
    loop.add_timer(100ms, [&] {
        if (++ticks >= 15) loop.stop();  // 1.5 seconds max
    });
    loop.run();

    CHECK(pool.connection_count() >= 2);
}

TEST_CASE("PgPool — execute simple query", "[pg_pool]")
{
    EventLoop loop;
    PgPool pool(loop, kConnInfo, 1, 2);
    pool.start();

    std::vector<PgResult> got_results;
    bool done = false;

    // Wait for pool to connect, then fire query
    loop.add_timer(200ms, [&] {
        pool.execute("SELECT 42::int4 AS answer",
            [&](std::vector<PgResult> res) {
                got_results = std::move(res);
                done = true;
                loop.stop();
            });
    }, /*repeat=*/false);

    loop.add_timer(3000ms, [&] { loop.stop(); }, false);
    loop.run();

    REQUIRE(done);
    REQUIRE(got_results.size() == 1);
    CHECK(got_results[0].ok());
    CHECK(std::string(got_results[0].value(0, 0)) == "42");
}

TEST_CASE("PgPool — execute multiple queries sequentially", "[pg_pool]")
{
    EventLoop loop;
    PgPool pool(loop, kConnInfo, 1, 2);
    pool.start();

    std::vector<std::string> values;
    int done_count = 0;

    loop.add_timer(200ms, [&] {
        for (int i = 1; i <= 3; ++i) {
            std::string sql = "SELECT " + std::to_string(i * 10) + "::int4 AS v";
            pool.execute(sql,
                [&, i](std::vector<PgResult> res) {
                    if (!res.empty() && res[0].ok())
                        values.push_back(res[0].value(0, 0));
                    if (++done_count == 3)
                        loop.stop();
                });
        }
    }, false);

    loop.add_timer(5000ms, [&] { loop.stop(); }, false);
    loop.run();

    CHECK(done_count == 3);
    CHECK(values.size() == 3);
    // All expected values present (order may differ)
    CHECK(std::find(values.begin(), values.end(), "10") != values.end());
    CHECK(std::find(values.begin(), values.end(), "20") != values.end());
    CHECK(std::find(values.begin(), values.end(), "30") != values.end());
}

TEST_CASE("PgPool — query queued when all connections busy", "[pg_pool]")
{
    EventLoop loop;
    // Only 1 connection, send 2 queries → second must queue
    PgPool pool(loop, kConnInfo, 1, 1);
    pool.start();

    int done_count = 0;
    std::vector<std::string> results;

    loop.add_timer(300ms, [&] {
        // Fire both queries immediately
        pool.execute("SELECT pg_sleep(0.05), 'A'::text AS v",
            [&](std::vector<PgResult> res) {
                if (!res.empty() && res[0].ok() && res[0].columns() >= 2)
                    results.push_back(res[0].value(0, 1));
                if (++done_count == 2) loop.stop();
            });
        pool.execute("SELECT 'B'::text AS v",
            [&](std::vector<PgResult> res) {
                if (!res.empty() && res[0].ok())
                    results.push_back(res[0].value(0, 0));
                if (++done_count == 2) loop.stop();
            });
        // Queue size should be 1 (second query waiting)
        CHECK(pool.queue_size() >= 1);
    }, false);

    loop.add_timer(6000ms, [&] { loop.stop(); }, false);
    loop.run();

    CHECK(done_count == 2);
    CHECK(results.size() == 2);
}

TEST_CASE("PgPool — error callback on bad SQL", "[pg_pool]")
{
    EventLoop loop;
    PgPool pool(loop, kConnInfo, 1, 2);
    pool.start();

    bool got_result   = false;
    bool got_error    = false;
    std::string error_msg;

    loop.add_timer(200ms, [&] {
        // Valid queries return results even if SQL error → error comes via non-ok status
        pool.execute("SELECT * FROM this_table_does_not_exist_xyz",
            [&](std::vector<PgResult> res) {
                // Results still delivered but not ok
                got_result = true;
                if (!res.empty() && !res[0].ok()) {
                    got_error = true;
                    error_msg = res[0].error_message();
                }
                loop.stop();
            });
    }, false);

    loop.add_timer(3000ms, [&] { loop.stop(); }, false);
    loop.run();

    CHECK(got_result);
    CHECK(got_error);
    CHECK(error_msg.size() > 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// LISTEN / NOTIFY
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("PgPool — listen receives notification from execute", "[pg_notify]")
{
    EventLoop loop;
    PgPool pool(loop, kConnInfo, 1, 2);

    bool     notified = false;
    std::string got_channel;
    std::string got_payload;

    pool.listen("test_apostol", [&](std::string_view ch, std::string_view payload) {
        notified     = true;
        got_channel  = ch;
        got_payload  = payload;
        loop.stop();
    });

    pool.start();

    // After 400ms the listener should be connected and LISTENing.
    // Send NOTIFY via the regular query pool.
    loop.add_timer(400ms, [&] {
        pool.execute("SELECT pg_notify('test_apostol', 'hello_world')",
            [](std::vector<PgResult>) {});
    }, false);

    loop.add_timer(4000ms, [&] { loop.stop(); }, false);
    loop.run();

    CHECK(notified);
    CHECK(got_channel  == "test_apostol");
    CHECK(got_payload  == "hello_world");
}

TEST_CASE("PgPool — listen multiple channels", "[pg_notify]")
{
    EventLoop loop;
    PgPool pool(loop, kConnInfo, 1, 2);

    int received = 0;
    std::vector<std::string> channels_received;

    pool.listen("chan_alpha", [&](std::string_view ch, std::string_view /*payload*/) {
        channels_received.push_back(std::string(ch));
        if (++received == 2) loop.stop();
    });
    pool.listen("chan_beta", [&](std::string_view ch, std::string_view /*payload*/) {
        channels_received.push_back(std::string(ch));
        if (++received == 2) loop.stop();
    });

    pool.start();

    loop.add_timer(400ms, [&] {
        pool.execute("SELECT pg_notify('chan_alpha', '1')", [](auto) {});
        pool.execute("SELECT pg_notify('chan_beta', '2')",  [](auto) {});
    }, false);

    loop.add_timer(4000ms, [&] { loop.stop(); }, false);
    loop.run();

    CHECK(received == 2);
    // Both channels received (order may differ)
    CHECK(std::find(channels_received.begin(), channels_received.end(), "chan_alpha")
          != channels_received.end());
    CHECK(std::find(channels_received.begin(), channels_received.end(), "chan_beta")
          != channels_received.end());
}

TEST_CASE("PgPool — listen called after start", "[pg_notify]")
{
    EventLoop loop;
    PgPool pool(loop, kConnInfo, 1, 2);
    pool.start();

    bool notified = false;

    // Register listener AFTER start()
    loop.add_timer(100ms, [&] {
        pool.listen("late_channel", [&](std::string_view, std::string_view payload) {
            notified = (payload == "late_payload");
            loop.stop();
        });
    }, false);

    // Fire NOTIFY after listener has had time to register
    loop.add_timer(600ms, [&] {
        pool.execute("SELECT pg_notify('late_channel', 'late_payload')", [](auto) {});
    }, false);

    loop.add_timer(4000ms, [&] { loop.stop(); }, false);
    loop.run();

    CHECK(notified);
}

TEST_CASE("PgPool — unlisten stops notifications", "[pg_notify]")
{
    EventLoop loop;
    PgPool pool(loop, kConnInfo, 1, 2);

    int received_before = 0;
    int received_after  = 0;
    bool unlistened     = false;

    pool.listen("unlisten_ch", [&](std::string_view, std::string_view) {
        if (!unlistened)
            ++received_before;
        else
            ++received_after;
    });

    pool.start();

    // Step 1: send first notification (should arrive)
    loop.add_timer(400ms, [&] {
        pool.execute("SELECT pg_notify('unlisten_ch', 'before')", [](auto) {});
    }, false);

    // Step 2: unlisten
    loop.add_timer(800ms, [&] {
        pool.unlisten("unlisten_ch");
        unlistened = true;
    }, false);

    // Step 3: send second notification (should NOT arrive after unlisten)
    loop.add_timer(1200ms, [&] {
        pool.execute("SELECT pg_notify('unlisten_ch', 'after')", [](auto) {});
    }, false);

    // Step 4: wait and stop
    loop.add_timer(2000ms, [&] { loop.stop(); }, false);
    loop.run();

    CHECK(received_before >= 1);
    CHECK(received_after  == 0);
}
