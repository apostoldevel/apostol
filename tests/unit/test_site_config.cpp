// tests/unit/test_site_config.cpp
// Unit tests for SiteConfig: load, find by hostname.

#include <catch2/catch_test_macros.hpp>

#include "apostol/site_config.hpp"

#include <filesystem>
#include <fstream>

using namespace apostol;
namespace fs = std::filesystem;

namespace {
struct TempDir {
    fs::path path;
    explicit TempDir(std::string_view name)
        : path(fs::temp_directory_path() / name)
    {
        fs::create_directories(path);
    }
    ~TempDir() { fs::remove_all(path); }

    void write(std::string_view filename, std::string_view content) const
    {
        std::ofstream f(path / filename);
        f << content;
    }
};
} // namespace

// ─── load ────────────────────────────────────────────────────────────────────

TEST_CASE("SiteConfigs: load from directory", "[site_config]")
{
    TempDir dir("apostol_test_sites_load");
    dir.write("default.json", R"({
        "hosts": ["localhost:4977", "example.com"],
        "root": "www",
        "oauth2": {
            "identifier": "/oauth/identifier",
            "callback": "/dashboard",
            "error": "/oauth/error"
        }
    })");

    SiteConfigs sites;
    sites.load(dir.path);

    REQUIRE(sites.configs().size() == 1);
    CHECK(sites.configs()[0].hosts.size() == 2);
    CHECK(sites.configs()[0].root == "www");
    CHECK(sites.configs()[0].oauth2.identifier == "/oauth/identifier");
    CHECK(sites.configs()[0].oauth2.callback == "/dashboard");
    CHECK(sites.configs()[0].oauth2.error == "/oauth/error");
}

TEST_CASE("SiteConfigs: load multiple files", "[site_config]")
{
    TempDir dir("apostol_test_sites_multi");
    dir.write("a.json", R"({
        "hosts": ["a.com"],
        "oauth2": {"identifier": "/a/login"}
    })");
    dir.write("b.json", R"({
        "hosts": ["b.com"],
        "oauth2": {"identifier": "/b/login"}
    })");

    SiteConfigs sites;
    sites.load(dir.path);

    REQUIRE(sites.configs().size() == 2);
}

TEST_CASE("SiteConfigs: load empty directory", "[site_config]")
{
    TempDir dir("apostol_test_sites_empty");

    SiteConfigs sites;
    sites.load(dir.path);
    CHECK(sites.configs().empty());
}

TEST_CASE("SiteConfigs: load non-existent directory", "[site_config]")
{
    SiteConfigs sites;
    sites.load("/tmp/apostol_no_such_sites_dir_99999");
    CHECK(sites.configs().empty());
}

TEST_CASE("SiteConfigs: load skips malformed JSON", "[site_config]")
{
    TempDir dir("apostol_test_sites_malformed");
    dir.write("broken.json", "not valid json");
    dir.write("good.json", R"({
        "hosts": ["good.com"],
        "oauth2": {"callback": "/ok"}
    })");

    SiteConfigs sites;
    sites.load(dir.path);
    REQUIRE(sites.configs().size() == 1);
    CHECK(sites.configs()[0].oauth2.callback == "/ok");
}

TEST_CASE("SiteConfigs: load with all oauth2 fields", "[site_config]")
{
    TempDir dir("apostol_test_sites_all_fields");
    dir.write("full.json", R"({
        "hosts": ["full.com"],
        "root": "public",
        "oauth2": {
            "identifier": "/login",
            "secret": "/password",
            "callback": "/success",
            "error": "/error",
            "debug": "http://localhost:3000"
        }
    })");

    SiteConfigs sites;
    sites.load(dir.path);

    REQUIRE(sites.configs().size() == 1);
    const auto& s = sites.configs()[0];
    CHECK(s.oauth2.identifier == "/login");
    CHECK(s.oauth2.secret == "/password");
    CHECK(s.oauth2.callback == "/success");
    CHECK(s.oauth2.error == "/error");
    CHECK(s.oauth2.debug == "http://localhost:3000");
}

// ─── find ────────────────────────────────────────────────────────────────────

TEST_CASE("SiteConfigs: find by hostname", "[site_config]")
{
    TempDir dir("apostol_test_sites_find");
    dir.write("default.json", R"({
        "hosts": ["localhost:4977", "example.com"],
        "oauth2": {"callback": "/dash"}
    })");

    SiteConfigs sites;
    sites.load(dir.path);

    auto* s1 = sites.find("localhost:4977");
    REQUIRE(s1 != nullptr);
    CHECK(s1->oauth2.callback == "/dash");

    auto* s2 = sites.find("example.com");
    REQUIRE(s2 != nullptr);
    CHECK(s2->oauth2.callback == "/dash");

    CHECK(sites.find("unknown.com") == nullptr);
}

TEST_CASE("SiteConfigs: find with multiple sites", "[site_config]")
{
    TempDir dir("apostol_test_sites_find_multi");
    dir.write("a.json", R"({
        "hosts": ["a.com"],
        "oauth2": {"callback": "/a-dash"}
    })");
    dir.write("b.json", R"({
        "hosts": ["b.com"],
        "oauth2": {"callback": "/b-dash"}
    })");

    SiteConfigs sites;
    sites.load(dir.path);

    auto* sa = sites.find("a.com");
    auto* sb = sites.find("b.com");
    REQUIRE(sa != nullptr);
    REQUIRE(sb != nullptr);
    CHECK(sa->oauth2.callback == "/a-dash");
    CHECK(sb->oauth2.callback == "/b-dash");
}

// ─── clear ───────────────────────────────────────────────────────────────────

TEST_CASE("SiteConfigs: clear removes all", "[site_config]")
{
    TempDir dir("apostol_test_sites_clear");
    dir.write("default.json", R"({"hosts": ["x.com"]})");

    SiteConfigs sites;
    sites.load(dir.path);
    REQUIRE(!sites.configs().empty());

    sites.clear();
    CHECK(sites.configs().empty());
    CHECK(sites.find("x.com") == nullptr);
}
