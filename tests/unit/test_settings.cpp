#include <catch2/catch_test_macros.hpp>
#include "apostol/settings.hpp"
#include "apostol/config.hpp"

#include <cstdlib>

using namespace apostol;

TEST_CASE("AppSettings: defaults come from CMake defines", "[settings]")
{
    AppSettings s;

    CHECK(s.name        == APP_NAME);
    CHECK(s.version     == APP_VERSION);
    CHECK(s.locale      == APP_DEFAULT_LOCALE);
    CHECK(s.server_port == APP_DEFAULT_PORT);
    CHECK(s.log_level   == APP_DEFAULT_LOG_LEVEL);
    CHECK(s.server_backlog == APP_DEFAULT_LISTEN_BACKLOG);
    CHECK(s.workers     == APP_DEFAULT_WORKERS);
    CHECK(!s.pid_file.empty());
    CHECK(!s.error_log.empty());
    CHECK(s.pid_file.is_absolute());
    CHECK(s.error_log.is_absolute());
}

TEST_CASE("AppSettings::validate: default settings are valid", "[settings]")
{
    AppSettings s;
    auto errors = s.validate();
    CHECK(errors.empty());
}

TEST_CASE("AppSettings::validate: invalid log level", "[settings]")
{
    AppSettings s;
    s.log_level = "verbose";
    auto errors = s.validate();
    REQUIRE(errors.size() >= 1);
    bool found = false;
    for (auto& e : errors) if (e.key == "log.level") found = true;
    CHECK(found);
}

TEST_CASE("AppSettings::validate: invalid port 0", "[settings]")
{
    AppSettings s;
    s.server_port = 0;
    auto errors = s.validate();
    REQUIRE(errors.size() >= 1);
    bool found = false;
    for (auto& e : errors) if (e.key == "server.port") found = true;
    CHECK(found);
}

TEST_CASE("AppSettings::validate: pool.max < pool.min", "[settings]")
{
    AppSettings s;
    s.pg_pool_min = 10;
    s.pg_pool_max = 3;
    auto errors = s.validate();
    bool found = false;
    for (auto& e : errors)
        if (e.key == "postgres.pool.max") found = true;
    CHECK(found);
}

TEST_CASE("AppSettings::populate: overrides from JSON config", "[settings]")
{
    auto cfg = Config::from_string(R"({
        "server": { "port": 9090, "backlog": 256 },
        "log": { "level": "debug" },
        "workers": 4
    })");

    AppSettings s;
    s.populate(cfg);

    CHECK(s.server_port    == 9090);
    CHECK(s.server_backlog == 256);
    CHECK(s.log_level      == "debug");
    CHECK(s.workers        == 4);
    // defaults remain for absent keys
    CHECK(s.server_listen  == APP_DEFAULT_LISTEN);
}

TEST_CASE("AppSettings::populate: partial config leaves defaults intact", "[settings]")
{
    auto cfg = Config::from_string(R"({ "workers": 2 })");
    AppSettings s;
    REQUIRE_NOTHROW(s.populate(cfg));
    CHECK(s.workers == 2);
    CHECK(s.server_port == APP_DEFAULT_PORT);
}

TEST_CASE("AppSettings::resolve: relative path prepends prefix", "[settings]")
{
    AppSettings s;
    s.prefix = "/srv/myapp/";

    auto p = s.resolve("logs/error.log");
    CHECK(p == std::filesystem::path("/srv/myapp/logs/error.log"));

    auto q = s.resolve("/var/log/error.log");
    CHECK(q == std::filesystem::path("/var/log/error.log"));
}

// ─── build_pg_conninfo tests ─────────────────────────────────────────────────

// Helper: clear PG-related env vars to prevent test pollution
struct PgEnvGuard
{
    static constexpr const char* vars[] = {
        "PGHOST", "PGHOSTADDR", "PGPORT", "PGDATABASE",
        "PGUSER", "PGPASSWORD", "PGSSLMODE",
        "PGHOSTAPI", "PGPORTAPI", "PGUSERAPI", "PGPASSWORDAPI",
        "PGUSERKERNEL", "PGPASSWORDKERNEL"
    };

    PgEnvGuard()  { for (auto* v : vars) ::unsetenv(v); }
    ~PgEnvGuard() { for (auto* v : vars) ::unsetenv(v); }
};

TEST_CASE("build_pg_conninfo: explicit conninfo takes priority", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "conninfo": "host=db1 port=5433 dbname=test",
                "host": "ignored",
                "dbname": "ignored"
            }
        }
    })");

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 10);
    CHECK(ci == "host=db1 port=5433 dbname=test");
}

TEST_CASE("build_pg_conninfo: builds from individual JSON params", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "host": "localhost",
                "port": "5432",
                "dbname": "mydb",
                "user": "daemon",
                "password": "secret"
            }
        }
    })");

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 10);
    CHECK(ci == "host=localhost port=5432 dbname=mydb user=daemon password=secret connect_timeout=10");
}

TEST_CASE("build_pg_conninfo: empty params produce empty result", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "host": "",
                "user": ""
            }
        }
    })");

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 10);
    CHECK(ci.empty());
}

TEST_CASE("build_pg_conninfo: env vars override JSON params", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "host": "json-host",
                "dbname": "json-db",
                "user": "json-user"
            }
        }
    })");

    ::setenv("PGHOST", "env-host", 1);
    ::setenv("PGUSER", "env-user", 1);

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 0);
    CHECK(ci == "host=env-host dbname=json-db user=env-user");
}

TEST_CASE("build_pg_conninfo: PGHOSTADDR overrides PGHOST", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({ "postgres": { "worker": {} } })");

    ::setenv("PGHOST", "host1", 1);
    ::setenv("PGHOSTADDR", "10.0.0.1", 1);

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 0);
    CHECK(ci == "host=10.0.0.1");
}

TEST_CASE("build_pg_conninfo: helper role-specific env vars", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "helper": {
                "user": "json-user"
            }
        }
    })");

    ::setenv("PGUSER", "base-user", 1);
    ::setenv("PGUSERAPI", "api-user", 1);
    ::setenv("PGHOSTAPI", "api-host", 1);

    auto ci = AppSettings::build_pg_conninfo(cfg, "helper", 0);
    CHECK(ci == "host=api-host user=api-user");
}

TEST_CASE("build_pg_conninfo: kernel role-specific env vars", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "kernel": {
                "host": "db-host",
                "user": "json-user"
            }
        }
    })");

    ::setenv("PGUSERKERNEL", "kernel-user", 1);
    ::setenv("PGPASSWORDKERNEL", "kernel-pass", 1);

    auto ci = AppSettings::build_pg_conninfo(cfg, "kernel", 0);
    CHECK(ci == "host=db-host user=kernel-user password=kernel-pass");
}

TEST_CASE("build_pg_conninfo: connect_timeout=0 is omitted", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": { "worker": { "host": "localhost" } }
    })");

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 0);
    CHECK(ci == "host=localhost");
    CHECK(ci.find("connect_timeout") == std::string::npos);
}

TEST_CASE("AppSettings::populate: fallback helper <- worker", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "host": "db-host",
                "dbname": "mydb",
                "user": "daemon",
                "password": "pass"
            }
        }
    })");

    AppSettings s;
    s.populate(cfg);

    CHECK(!s.pg_conninfo_worker.empty());
    CHECK(s.pg_conninfo_helper == s.pg_conninfo_worker);
}

TEST_CASE("AppSettings::populate: fallback kernel <- worker", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "host": "db-host",
                "user": "daemon"
            }
        }
    })");

    AppSettings s;
    s.populate(cfg);

    CHECK(!s.pg_conninfo_worker.empty());
    CHECK(s.pg_conninfo_kernel == s.pg_conninfo_worker);
}

TEST_CASE("AppSettings::populate: helper has own params, no fallback", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "host": "db-host",
                "user": "daemon"
            },
            "helper": {
                "host": "api-host",
                "user": "apibot"
            }
        }
    })");

    AppSettings s;
    s.populate(cfg);

    CHECK(s.pg_conninfo_worker.find("daemon") != std::string::npos);
    CHECK(s.pg_conninfo_helper.find("apibot") != std::string::npos);
    CHECK(s.pg_conninfo_helper != s.pg_conninfo_worker);
}

TEST_CASE("build_pg_conninfo: missing role section returns empty", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({ "postgres": {} })");

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 10);
    CHECK(ci.empty());
}

TEST_CASE("build_pg_conninfo: partial params produce partial conninfo", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": { "user": "myuser" }
        }
    })");

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 5);
    CHECK(ci == "user=myuser connect_timeout=5");
}

TEST_CASE("build_pg_conninfo: numeric port is handled", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "host": "127.0.0.1",
                "port": 5432,
                "dbname": "web",
                "user": "http",
                "password": "http"
            }
        }
    })");

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 10);
    CHECK(ci == "host=127.0.0.1 port=5432 dbname=web user=http password=http connect_timeout=10");
}

TEST_CASE("build_pg_conninfo: boolean sslmode false -> disable", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "host": "db",
                "port": 5432,
                "sslmode": false
            }
        }
    })");

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 0);
    CHECK(ci == "host=db port=5432 sslmode=disable");
}

TEST_CASE("build_pg_conninfo: boolean sslmode true -> require", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "host": "db",
                "sslmode": true
            }
        }
    })");

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 0);
    CHECK(ci == "host=db sslmode=require");
}

TEST_CASE("build_pg_conninfo: string sslmode passed through", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "host": "db",
                "sslmode": "verify-full"
            }
        }
    })");

    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 0);
    CHECK(ci == "host=db sslmode=verify-full");
}

TEST_CASE("build_pg_conninfo: PGSSLMODE env overrides config", "[settings]")
{
    PgEnvGuard guard;
    auto cfg = Config::from_string(R"({
        "postgres": {
            "worker": {
                "host": "db",
                "sslmode": "require"
            }
        }
    })");

    ::setenv("PGSSLMODE", "disable", 1);
    auto ci = AppSettings::build_pg_conninfo(cfg, "worker", 0);
    CHECK(ci == "host=db sslmode=disable");
    ::unsetenv("PGSSLMODE");
}

// ─── log rotation settings tests ──────────────────────────────────────────────

TEST_CASE("AppSettings: log rotation defaults", "[settings]")
{
    AppSettings s;
    CHECK(s.log_keep_rotated == 5);
    CHECK(s.log_compress == true);
}

TEST_CASE("AppSettings::populate: log rotation settings from JSON", "[settings]")
{
    auto cfg = Config::from_string(R"({
        "log": {
            "keep_rotated": 10,
            "compress": false
        }
    })");

    AppSettings s;
    s.populate(cfg);
    CHECK(s.log_keep_rotated == 10);
    CHECK(s.log_compress == false);
}

TEST_CASE("AppSettings::populate: partial log rotation keeps defaults", "[settings]")
{
    auto cfg = Config::from_string(R"({
        "log": {
            "keep_rotated": 3
        }
    })");

    AppSettings s;
    s.populate(cfg);
    CHECK(s.log_keep_rotated == 3);
    CHECK(s.log_compress == true);  // default preserved
}
