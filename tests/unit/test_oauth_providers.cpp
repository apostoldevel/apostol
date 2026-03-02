// tests/unit/test_oauth_providers.cpp
// Unit tests for OAuthProviders: load, find_by_client_id, credentials, allowed_origins.

#include <catch2/catch_test_macros.hpp>

#include "apostol/oauth_providers.hpp"

#include <filesystem>
#include <fstream>

using namespace apostol;
namespace fs = std::filesystem;

// ─── RAII temp dir ───────────────────────────────────────────────────────────

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

TEST_CASE("OAuthProviders: load from directory with two JSON files", "[oauth]")
{
    TempDir dir("apostol_test_oauth_load");
    dir.write("default.json", R"({
        "web": {
            "client_id": "web-example.com",
            "client_secret": "web-secret",
            "algorithm": "HS256",
            "issuers": ["accounts.example.com"],
            "javascript_origins": ["https://example.com", "http://localhost:3000"]
        },
        "service": {
            "client_id": "service-example.com",
            "client_secret": "service-secret",
            "algorithm": "HS256",
            "issuers": ["accounts.example.com"],
            "javascript_origins": ["https://example.com"]
        }
    })");
    dir.write("google.json", R"({
        "web": {
            "client_id": "google-web-id",
            "client_secret": "google-secret",
            "algorithm": "HS384",
            "issuers": ["accounts.google.com"]
        }
    })");

    OAuthProviders providers;
    providers.load(dir.path);

    REQUIRE(providers.apps().size() == 3);
}

TEST_CASE("OAuthProviders: load preserves provider name from filename", "[oauth]")
{
    TempDir dir("apostol_test_oauth_provider_name");
    dir.write("myapp.json", R"({
        "web": {"client_id": "cid", "client_secret": "sec"}
    })");

    OAuthProviders providers;
    providers.load(dir.path);

    REQUIRE(providers.apps().size() == 1);
    REQUIRE(providers.apps()[0].provider == "myapp");
    REQUIRE(providers.apps()[0].name == "web");
}

TEST_CASE("OAuthProviders: load empty directory", "[oauth]")
{
    TempDir dir("apostol_test_oauth_empty");

    OAuthProviders providers;
    providers.load(dir.path);
    REQUIRE(providers.apps().empty());
}

TEST_CASE("OAuthProviders: load non-existent directory", "[oauth]")
{
    OAuthProviders providers;
    providers.load("/tmp/apostol_no_such_dir_xyzzy_99999");
    REQUIRE(providers.apps().empty());
}

TEST_CASE("OAuthProviders: load skips malformed JSON", "[oauth]")
{
    TempDir dir("apostol_test_oauth_malformed");
    dir.write("broken.json", "not { valid json !!!");
    dir.write("good.json", R"({"web": {"client_id": "good-id", "client_secret": "s"}})");

    OAuthProviders providers;
    providers.load(dir.path);

    REQUIRE(providers.apps().size() == 1);
    REQUIRE(providers.apps()[0].client_id == "good-id");
}

TEST_CASE("OAuthProviders: load skips non-json files", "[oauth]")
{
    TempDir dir("apostol_test_oauth_nonjson");
    dir.write("readme.txt", "ignore me");
    dir.write("valid.json", R"({"web": {"client_id": "cid", "client_secret": "sec"}})");

    OAuthProviders providers;
    providers.load(dir.path);
    REQUIRE(providers.apps().size() == 1);
}

TEST_CASE("OAuthProviders: load handles sections without optional fields", "[oauth]")
{
    TempDir dir("apostol_test_oauth_minimal");
    dir.write("min.json", R"({
        "basic": {"client_id": "basic-id"}
    })");

    OAuthProviders providers;
    providers.load(dir.path);
    REQUIRE(providers.apps().size() == 1);
    REQUIRE(providers.apps()[0].client_id == "basic-id");
    REQUIRE(providers.apps()[0].client_secret.empty());
    REQUIRE(providers.apps()[0].algorithm.empty());
    REQUIRE(providers.apps()[0].issuers.empty());
    REQUIRE(providers.apps()[0].javascript_origins.empty());
}

// ─── clear ───────────────────────────────────────────────────────────────────

TEST_CASE("OAuthProviders: clear removes all apps", "[oauth]")
{
    TempDir dir("apostol_test_oauth_clear");
    dir.write("a.json", R"({"web": {"client_id": "cid", "client_secret": "s"}})");

    OAuthProviders providers;
    providers.load(dir.path);
    REQUIRE(!providers.apps().empty());

    providers.clear();
    REQUIRE(providers.apps().empty());
}

// ─── find_by_client_id ───────────────────────────────────────────────────────

TEST_CASE("OAuthProviders: find_by_client_id found", "[oauth]")
{
    TempDir dir("apostol_test_oauth_find");
    dir.write("default.json", R"({
        "web": {"client_id": "web-id", "client_secret": "web-sec", "algorithm": "HS256"},
        "service": {"client_id": "svc-id", "client_secret": "svc-sec"}
    })");

    OAuthProviders providers;
    providers.load(dir.path);

    auto* app = providers.find_by_client_id("svc-id");
    REQUIRE(app != nullptr);
    REQUIRE(app->name == "service");
    REQUIRE(app->client_secret == "svc-sec");
}

TEST_CASE("OAuthProviders: find_by_client_id not found", "[oauth]")
{
    TempDir dir("apostol_test_oauth_find_miss");
    dir.write("default.json", R"({"web": {"client_id": "web-id", "client_secret": "s"}})");

    OAuthProviders providers;
    providers.load(dir.path);

    REQUIRE(providers.find_by_client_id("unknown-id") == nullptr);
}

// ─── credentials ─────────────────────────────────────────────────────────────

TEST_CASE("OAuthProviders: credentials found", "[oauth]")
{
    TempDir dir("apostol_test_oauth_creds");
    dir.write("default.json", R"({
        "service": {"client_id": "svc-id", "client_secret": "svc-secret"},
        "web": {"client_id": "web-id", "client_secret": "web-secret"}
    })");

    OAuthProviders providers;
    providers.load(dir.path);

    auto [id, secret] = providers.credentials("service");
    REQUIRE(id == "svc-id");
    REQUIRE(secret == "svc-secret");
}

TEST_CASE("OAuthProviders: credentials not found", "[oauth]")
{
    TempDir dir("apostol_test_oauth_creds_miss");
    dir.write("default.json", R"({"web": {"client_id": "w", "client_secret": "s"}})");

    OAuthProviders providers;
    providers.load(dir.path);

    auto [id, secret] = providers.credentials("nonexistent");
    REQUIRE(id.empty());
    REQUIRE(secret.empty());
}

TEST_CASE("OAuthProviders: credentials skips entries with empty secret", "[oauth]")
{
    TempDir dir("apostol_test_oauth_creds_empty");
    dir.write("default.json", R"({"service": {"client_id": "svc-id"}})");

    OAuthProviders providers;
    providers.load(dir.path);

    auto [id, secret] = providers.credentials("service");
    REQUIRE(id.empty());
}

// ─── allowed_origins ─────────────────────────────────────────────────────────

TEST_CASE("OAuthProviders: allowed_origins collects from all apps", "[oauth]")
{
    TempDir dir("apostol_test_oauth_origins");
    dir.write("default.json", R"({
        "web": {"client_id": "w", "client_secret": "s",
                "javascript_origins": ["https://a.com", "https://b.com"]},
        "service": {"client_id": "s", "client_secret": "s",
                    "javascript_origins": ["https://c.com"]}
    })");

    OAuthProviders providers;
    providers.load(dir.path);

    auto origins = providers.allowed_origins();
    REQUIRE(origins.size() == 3);
}

TEST_CASE("OAuthProviders: allowed_origins deduplicates", "[oauth]")
{
    TempDir dir("apostol_test_oauth_origins_dedup");
    dir.write("a.json", R"({"web": {"client_id": "w", "client_secret": "s",
                             "javascript_origins": ["https://shared.com"]}})");
    dir.write("b.json", R"({"svc": {"client_id": "s", "client_secret": "s",
                             "javascript_origins": ["https://shared.com", "https://unique.com"]}})");

    OAuthProviders providers;
    providers.load(dir.path);

    auto origins = providers.allowed_origins();
    // shared.com appears in both but should be deduplicated
    int shared_count = 0;
    for (const auto& o : origins)
        if (o == "https://shared.com") ++shared_count;
    REQUIRE(shared_count == 1);
    REQUIRE(origins.size() == 2);
}

TEST_CASE("OAuthProviders: allowed_origins empty when no origins", "[oauth]")
{
    TempDir dir("apostol_test_oauth_origins_empty");
    dir.write("default.json", R"({"web": {"client_id": "w", "client_secret": "s"}})");

    OAuthProviders providers;
    providers.load(dir.path);

    REQUIRE(providers.allowed_origins().empty());
}
