#include <catch2/catch_test_macros.hpp>

#include "apostol/config.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace apostol;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static Config from_json(std::string_view s)
{
    return Config::from_string(s);
}

// ─── Basic parsing ────────────────────────────────────────────────────────────

TEST_CASE("Config parses flat string values", "[config]")
{
    auto cfg = from_json(R"({"name":"apostol","version":"1.0"})");
    REQUIRE(cfg.get_string("name") == "apostol");
    REQUIRE(cfg.get_string("version") == "1.0");
}

TEST_CASE("Config parses nested values via dot notation", "[config]")
{
    auto cfg = from_json(R"({
        "server": { "host": "0.0.0.0", "port": 8080 }
    })");
    REQUIRE(cfg.get_string("server.host") == "0.0.0.0");
    REQUIRE(cfg.get_int("server.port") == 8080);
}

TEST_CASE("Config parses boolean values", "[config]")
{
    auto cfg = from_json(R"({"daemon":true,"debug":false})");
    REQUIRE(cfg.get_bool("daemon") == true);
    REQUIRE(cfg.get_bool("debug") == false);
}

TEST_CASE("Config returns default values for missing keys", "[config]")
{
    auto cfg = from_json(R"({"x":1})");
    REQUIRE(cfg.get_string("missing", "default") == "default");
    REQUIRE(cfg.get_int("missing", 99) == 99);
    REQUIRE(cfg.get_bool("missing", true) == true);
}

TEST_CASE("Config get_string throws on missing required key", "[config]")
{
    auto cfg = from_json(R"({})");
    REQUIRE_THROWS_AS(cfg.get_string("does.not.exist"), ConfigError);
}

TEST_CASE("Config get_int throws on missing required key", "[config]")
{
    auto cfg = from_json(R"({})");
    REQUIRE_THROWS_AS(cfg.get_int("missing"), ConfigError);
}

TEST_CASE("Config throws on type mismatch", "[config]")
{
    auto cfg = from_json(R"({"port":"not-a-number"})");
    REQUIRE_THROWS_AS(cfg.get_int("port"), ConfigError);
}

TEST_CASE("Config has() returns correct presence", "[config]")
{
    auto cfg = from_json(R"({"a":{"b":1}})");
    REQUIRE(cfg.has("a") == true);
    REQUIRE(cfg.has("a.b") == true);
    REQUIRE(cfg.has("a.c") == false);
    REQUIRE(cfg.has("x") == false);
}

// ─── get_section ─────────────────────────────────────────────────────────────

TEST_CASE("Config get_section returns sub-config", "[config]")
{
    auto cfg = from_json(R"({"db":{"host":"localhost","port":5432}})");
    auto db = cfg.get_section("db");
    REQUIRE(db.get_string("host") == "localhost");
    REQUIRE(db.get_int("port") == 5432);
}

TEST_CASE("Config get_section throws on missing section", "[config]")
{
    auto cfg = from_json(R"({})");
    REQUIRE_THROWS_AS(cfg.get_section("db"), ConfigError);
}

// ─── Environment variable expansion ─────────────────────────────────────────

TEST_CASE("Config expands ${VAR} references", "[config]")
{
    ::setenv("TEST_HOST", "myhost", 1);
    auto cfg = from_json(R"({"host":"${TEST_HOST}"})");
    REQUIRE(cfg.get_string("host") == "myhost");
    ::unsetenv("TEST_HOST");
}

TEST_CASE("Config expands $VAR references", "[config]")
{
    ::setenv("TEST_PORT", "9090", 1);
    auto cfg = from_json(R"({"addr":"host:$TEST_PORT"})");
    REQUIRE(cfg.get_string("addr") == "host:9090");
    ::unsetenv("TEST_PORT");
}

TEST_CASE("Config substitutes empty string for unset env vars", "[config]")
{
    ::unsetenv("APOSTOL_UNSET_VAR");
    auto cfg = from_json(R"({"val":"prefix_${APOSTOL_UNSET_VAR}_suffix"})");
    REQUIRE(cfg.get_string("val") == "prefix__suffix");
}

TEST_CASE("Config expands vars in nested objects", "[config]")
{
    ::setenv("DB_NAME", "mydb", 1);
    auto cfg = from_json(R"({"db":{"name":"${DB_NAME}"}})");
    REQUIRE(cfg.get_string("db.name") == "mydb");
    ::unsetenv("DB_NAME");
}

// ─── File loading ─────────────────────────────────────────────────────────────

TEST_CASE("Config loads from file", "[config][file]")
{
    auto tmp = fs::temp_directory_path() / "apostol_test_config.json";

    {
        std::ofstream f(tmp);
        f << R"({"app":"test","workers":4})";
    }

    auto cfg = Config::from_file(tmp);
    REQUIRE(cfg.get_string("app") == "test");
    REQUIRE(cfg.get_int("workers") == 4);

    fs::remove(tmp);
}

TEST_CASE("Config throws on missing file", "[config][file]")
{
    REQUIRE_THROWS_AS(Config::from_file("/tmp/apostol_does_not_exist.json"), ConfigError);
}

TEST_CASE("Config throws on invalid JSON", "[config]")
{
    REQUIRE_THROWS_AS(Config::from_string("{invalid json}"), ConfigError);
}

TEST_CASE("Config reload re-reads file", "[config][file]")
{
    auto tmp = fs::temp_directory_path() / "apostol_test_reload.json";

    {
        std::ofstream f(tmp);
        f << R"({"version":1})";
    }

    auto cfg = Config::from_file(tmp);
    REQUIRE(cfg.get_int("version") == 1);

    {
        std::ofstream f(tmp);
        f << R"({"version":2})";
    }

    cfg.reload();
    REQUIRE(cfg.get_int("version") == 2);

    fs::remove(tmp);
}
