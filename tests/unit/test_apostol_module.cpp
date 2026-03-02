// tests/unit/test_apostol_module.cpp
// Unit tests for ApostolModule: method dispatch, CORS, HTTP utilities, PG helpers.

#include <catch2/catch_test_macros.hpp>

#include "apostol/apostol_module.hpp"
#include "apostol/http.hpp"
#include "apostol/oauth_providers.hpp"
#ifdef WITH_POSTGRESQL
#include "apostol/pg.hpp"
#include <libpq-fe.h>
#endif

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

using namespace apostol;

// ─── Test doubles ─────────────────────────────────────────────────────────────

// Concrete ApostolModule used for testing basic dispatch and CORS.
class EchoModule final : public ApostolModule
{
public:
    std::string_view name()    const override { return "echo"; }
    bool             enabled() const override { return true;  }

    // Expose protected helpers for testing via using-declarations
    using ApostolModule::get_host;
    using ApostolModule::get_origin;
    using ApostolModule::get_real_ip;
    using ApostolModule::get_protocol;
    using ApostolModule::redirect;
    using ApostolModule::reply_error;
    using ApostolModule::add_allowed_origin;
    using ApostolModule::load_allowed_origins;
    using ApostolModule::is_origin_allowed;

    // Additional HTTP utilities
    using ApostolModule::get_user_agent;
    using ApostolModule::content_to_json;
    using ApostolModule::parse_authorization;
    using ApostolModule::url_encode;
    using ApostolModule::url_decode;
    using ApostolModule::json_escape;
    using ApostolModule::match_path;

#ifdef WITH_POSTGRESQL
    using ApostolModule::pg_result_to_json;
    using ApostolModule::reply_pg;
    using ApostolModule::pq_quote_literal;
    using ApostolModule::headers_to_json;
    using ApostolModule::params_to_json;
    using ApostolModule::check_pg_error;
    using ApostolModule::error_code_to_status;
#endif

protected:
    void init_methods() override
    {
        add_method("GET",  [](const HttpRequest& req, HttpResponse& resp) {
            resp.set_status(HttpStatus::ok).set_body(req.path);
        });
        add_method("POST", [](const HttpRequest& req, HttpResponse& resp) {
            resp.set_status(HttpStatus::ok).set_body(req.body);
        });
    }
};

// Module with CORS configured for https://example.com
class CorsModule final : public ApostolModule
{
public:
    CorsModule()
    {
        add_allowed_origin("https://example.com");
        add_allowed_header("Authorization");
    }

    std::string_view name()    const override { return "cors"; }
    bool             enabled() const override { return true;  }

    using ApostolModule::is_origin_allowed;
    using ApostolModule::cors;

protected:
    void init_methods() override
    {
        add_method("GET", [](const HttpRequest&, HttpResponse& resp) {
            resp.set_status(HttpStatus::ok).set_body("ok");
        });
    }
};

// Module with wildcard CORS
class WildcardCorsModule final : public ApostolModule
{
public:
    WildcardCorsModule() { add_allowed_origin("*"); }

    std::string_view name()    const override { return "wildcard"; }
    bool             enabled() const override { return true; }

    using ApostolModule::is_origin_allowed;

protected:
    void init_methods() override
    {
        add_method("GET", [](const HttpRequest&, HttpResponse& resp) {
            resp.set_status(HttpStatus::ok).set_body("ok");
        });
    }
};

// ─── Helpers ──────────────────────────────────────────────────────────────────

static HttpRequest make_request(std::string method, std::string path,
                                 std::vector<std::pair<std::string,std::string>> headers = {},
                                 std::string body = "")
{
    HttpRequest req;
    req.method  = std::move(method);
    req.path    = std::move(path);
    req.version = "HTTP/1.1";
    req.headers = std::move(headers);
    req.body    = std::move(body);
    return req;
}

// ─── Method dispatch ──────────────────────────────────────────────────────────

TEST_CASE("ApostolModule: GET handler is called", "[apostol_module]")
{
    EchoModule mod;
    HttpRequest  req = make_request("GET", "/hello");
    HttpResponse resp;

    REQUIRE(mod.execute(req, resp));
    REQUIRE(resp.serialize().find("HTTP/1.1 200 OK") != std::string::npos);
    REQUIRE(resp.serialize().find("/hello") != std::string::npos);
}

TEST_CASE("ApostolModule: POST handler is called with body", "[apostol_module]")
{
    EchoModule mod;
    HttpRequest  req = make_request("POST", "/submit", {}, "payload");
    HttpResponse resp;

    REQUIRE(mod.execute(req, resp));
    REQUIRE(resp.serialize().find("payload") != std::string::npos);
}

TEST_CASE("ApostolModule: unknown method returns 405 with Allow header", "[apostol_module]")
{
    EchoModule mod;
    HttpRequest  req = make_request("DELETE", "/resource");
    HttpResponse resp;

    REQUIRE(mod.execute(req, resp));
    std::string s = resp.serialize();
    REQUIRE(s.find("HTTP/1.1 405") != std::string::npos);
    REQUIRE(s.find("Allow:") != std::string::npos);
}

TEST_CASE("ApostolModule: Allow header contains registered methods + OPTIONS", "[apostol_module]")
{
    EchoModule mod;
    HttpRequest  req = make_request("DELETE", "/x");
    HttpResponse resp;

    mod.execute(req, resp);
    std::string s = resp.serialize();
    REQUIRE(s.find("GET") != std::string::npos);
    REQUIRE(s.find("POST") != std::string::npos);
    REQUIRE(s.find("OPTIONS") != std::string::npos);
}

TEST_CASE("ApostolModule: OPTIONS returns 204 No Content with Allow header", "[apostol_module]")
{
    EchoModule mod;
    HttpRequest  req = make_request("OPTIONS", "/");
    HttpResponse resp;

    REQUIRE(mod.execute(req, resp));
    std::string s = resp.serialize();
    REQUIRE(s.find("HTTP/1.1 204 No Content") != std::string::npos);
    REQUIRE(s.find("Allow:") != std::string::npos);
}

TEST_CASE("ApostolModule: init_methods is called lazily on first execute", "[apostol_module]")
{
    // Two separate module instances to verify independent initialization
    EchoModule a, b;

    HttpRequest  req  = make_request("GET", "/x");
    HttpResponse resp;

    REQUIRE(a.execute(req, resp));
    REQUIRE(b.execute(req, resp));
    // Both should work independently
}

// ─── CORS ─────────────────────────────────────────────────────────────────────

TEST_CASE("ApostolModule: no CORS headers when allowed_origins is empty", "[apostol_module]")
{
    EchoModule mod;
    HttpRequest req = make_request("GET", "/",
        {{"Origin", "https://example.com"}});
    HttpResponse resp;

    mod.execute(req, resp);
    std::string s = resp.serialize();
    REQUIRE(s.find("Access-Control-Allow-Origin") == std::string::npos);
}

TEST_CASE("ApostolModule: CORS headers added for allowed origin", "[apostol_module]")
{
    CorsModule mod;
    HttpRequest req = make_request("GET", "/",
        {{"Origin", "https://example.com"}});
    HttpResponse resp;

    mod.execute(req, resp);
    std::string s = resp.serialize();
    REQUIRE(s.find("Access-Control-Allow-Origin: https://example.com") != std::string::npos);
    REQUIRE(s.find("Access-Control-Allow-Methods:") != std::string::npos);
    REQUIRE(s.find("Access-Control-Allow-Headers:") != std::string::npos);
    REQUIRE(s.find("Access-Control-Allow-Credentials: true") != std::string::npos);
    REQUIRE(s.find("Vary: Origin") != std::string::npos);
}

TEST_CASE("ApostolModule: CORS headers NOT added for unknown origin", "[apostol_module]")
{
    CorsModule mod;
    HttpRequest req = make_request("GET", "/",
        {{"Origin", "https://evil.com"}});
    HttpResponse resp;

    mod.execute(req, resp);
    std::string s = resp.serialize();
    REQUIRE(s.find("Access-Control-Allow-Origin") == std::string::npos);
}

TEST_CASE("ApostolModule: CORS headers NOT added when Origin header absent", "[apostol_module]")
{
    CorsModule mod;
    HttpRequest  req = make_request("GET", "/");
    HttpResponse resp;

    mod.execute(req, resp);
    REQUIRE(resp.serialize().find("Access-Control-Allow-Origin") == std::string::npos);
}

TEST_CASE("ApostolModule: CORS wildcard allows any origin", "[apostol_module]")
{
    WildcardCorsModule mod;
    REQUIRE(mod.is_origin_allowed("https://random.example.org"));
    REQUIRE(mod.is_origin_allowed("http://localhost:3000"));
}

TEST_CASE("ApostolModule: add_allowed_header appears in Allow-Headers", "[apostol_module]")
{
    CorsModule mod;
    HttpRequest req = make_request("GET", "/",
        {{"Origin", "https://example.com"}});
    HttpResponse resp;

    mod.execute(req, resp);
    std::string s = resp.serialize();
    // Default headers + added "Authorization"
    REQUIRE(s.find("Content-Type") != std::string::npos);
    REQUIRE(s.find("X-Requested-With") != std::string::npos);
    REQUIRE(s.find("Authorization") != std::string::npos);
}

TEST_CASE("ApostolModule: CORS headers added for OPTIONS preflight", "[apostol_module]")
{
    CorsModule mod;
    HttpRequest req = make_request("OPTIONS", "/",
        {{"Origin", "https://example.com"}});
    HttpResponse resp;

    mod.execute(req, resp);
    std::string s = resp.serialize();
    REQUIRE(s.find("Access-Control-Allow-Origin: https://example.com") != std::string::npos);
    REQUIRE(s.find("HTTP/1.1 204") != std::string::npos);
}

TEST_CASE("ApostolModule: is_origin_allowed returns false for empty list", "[apostol_module]")
{
    EchoModule mod;
    // EchoModule adds no allowed origins — no public accessor, use through CORS test
    // Verify indirectly: no CORS headers when origin not in list
    HttpRequest req = make_request("GET", "/", {{"Origin", "https://x.com"}});
    HttpResponse resp;
    mod.execute(req, resp);
    REQUIRE(resp.serialize().find("Access-Control-Allow-Origin") == std::string::npos);
}

// ─── HTTP utilities ───────────────────────────────────────────────────────────

TEST_CASE("ApostolModule: get_host returns Host header or localhost", "[apostol_module]")
{
    HttpRequest req = make_request("GET", "/", {{"Host", "api.example.com"}});
    REQUIRE(EchoModule::get_host(req) == "api.example.com");

    HttpRequest req2 = make_request("GET", "/");
    REQUIRE(EchoModule::get_host(req2) == "localhost");
}

TEST_CASE("ApostolModule: get_origin returns Origin header", "[apostol_module]")
{
    HttpRequest req = make_request("GET", "/", {{"Origin", "https://app.com"}});
    REQUIRE(EchoModule::get_origin(req) == "https://app.com");

    HttpRequest req2 = make_request("GET", "/");
    REQUIRE(EchoModule::get_origin(req2) == "");
}

TEST_CASE("ApostolModule: get_real_ip returns X-Real-IP header", "[apostol_module]")
{
    HttpRequest req = make_request("GET", "/", {{"X-Real-IP", "1.2.3.4"}});
    REQUIRE(EchoModule::get_real_ip(req) == "1.2.3.4");

    HttpRequest req2 = make_request("GET", "/");
    REQUIRE(EchoModule::get_real_ip(req2) == "");
}

TEST_CASE("ApostolModule: get_protocol returns X-Forwarded-Proto or http", "[apostol_module]")
{
    HttpRequest req = make_request("GET", "/", {{"X-Forwarded-Proto", "https"}});
    REQUIRE(EchoModule::get_protocol(req) == "https");

    HttpRequest req2 = make_request("GET", "/");
    REQUIRE(EchoModule::get_protocol(req2) == "http");
}

TEST_CASE("ApostolModule: redirect sets Location and 302 by default", "[apostol_module]")
{
    HttpResponse resp;
    EchoModule::redirect(resp, "/new/path");
    std::string s = resp.serialize();
    REQUIRE(s.find("HTTP/1.1 302 Found") != std::string::npos);
    REQUIRE(s.find("Location: /new/path") != std::string::npos);
}

TEST_CASE("ApostolModule: redirect with 301 Moved Permanently", "[apostol_module]")
{
    HttpResponse resp;
    EchoModule::redirect(resp, "https://new.example.com", HttpStatus::moved_permanently);
    REQUIRE(resp.serialize().find("HTTP/1.1 301 Moved Permanently") != std::string::npos);
}

TEST_CASE("ApostolModule: reply_error produces JSON error body", "[apostol_module]")
{
    HttpResponse resp;
    EchoModule::reply_error(resp, HttpStatus::not_found, "page not found");
    std::string s = resp.serialize();
    REQUIRE(s.find("HTTP/1.1 404 Not Found") != std::string::npos);
    REQUIRE(s.find("Content-Type: application/json") != std::string::npos);
    REQUIRE(s.find(R"("code":404)") != std::string::npos);
    REQUIRE(s.find(R"("message":"page not found")") != std::string::npos);
}

TEST_CASE("ApostolModule: reply_error with int code", "[apostol_module]")
{
    HttpResponse resp;
    EchoModule::reply_error(resp, 422, "validation failed");
    std::string s = resp.serialize();
    REQUIRE(s.find("HTTP/1.1 422") != std::string::npos);
    REQUIRE(s.find(R"("code":422)") != std::string::npos);
    REQUIRE(s.find(R"("message":"validation failed")") != std::string::npos);
}

TEST_CASE("ApostolModule: reply_error escapes JSON special chars in message", "[apostol_module]")
{
    HttpResponse resp;
    EchoModule::reply_error(resp, HttpStatus::bad_request, "bad \"input\" here");
    REQUIRE(resp.serialize().find(R"(bad \"input\" here)") != std::string::npos);
}

// ─── Additional HTTP utility delegates ────────────────────────────────────────

TEST_CASE("ApostolModule: get_user_agent returns User-Agent header", "[apostol_module]")
{
    HttpRequest req = make_request("GET", "/", {{"User-Agent", "TestBot/1.0"}});
    REQUIRE(EchoModule::get_user_agent(req) == "TestBot/1.0");

    HttpRequest req2 = make_request("GET", "/");
    REQUIRE(EchoModule::get_user_agent(req2) == "");
    REQUIRE(EchoModule::get_user_agent(req2, "default-agent") == "default-agent");
}

TEST_CASE("ApostolModule: url_encode encodes special characters", "[apostol_module]")
{
    REQUIRE(EchoModule::url_encode("hello world") == "hello%20world");
    REQUIRE(EchoModule::url_encode("a+b=c") == "a%2Bb%3Dc");
    REQUIRE(EchoModule::url_encode("simple") == "simple");
}

TEST_CASE("ApostolModule: url_decode decodes percent-encoding", "[apostol_module]")
{
    REQUIRE(EchoModule::url_decode("hello%20world") == "hello world");
    REQUIRE(EchoModule::url_decode("a+b") == "a b");
    REQUIRE(EchoModule::url_decode("simple") == "simple");
}

TEST_CASE("ApostolModule: json_escape escapes special characters", "[apostol_module]")
{
    REQUIRE(EchoModule::json_escape(R"(say "hello")") == R"(say \"hello\")");
    REQUIRE(EchoModule::json_escape("line\nnewline") == "line\\nnewline");
    REQUIRE(EchoModule::json_escape("tab\there") == "tab\\there");
    REQUIRE(EchoModule::json_escape("plain") == "plain");
}

TEST_CASE("ApostolModule: match_path matches exact and glob patterns", "[apostol_module]")
{
    std::vector<std::string> patterns = {"/api/*", "/health"};
    REQUIRE(EchoModule::match_path("/api/v1/users", patterns));
    REQUIRE(EchoModule::match_path("/health", patterns));
    REQUIRE_FALSE(EchoModule::match_path("/other", patterns));
}

TEST_CASE("ApostolModule: parse_authorization parses Bearer token", "[apostol_module]")
{
    auto auth = EchoModule::parse_authorization("Bearer abc123");
    REQUIRE(auth.schema == Authorization::Schema::bearer);
    REQUIRE(auth.token == "abc123");
}

TEST_CASE("ApostolModule: parse_authorization returns none for empty", "[apostol_module]")
{
    auto auth = EchoModule::parse_authorization("");
    REQUIRE(auth.schema == Authorization::Schema::none);
}

TEST_CASE("ApostolModule: content_to_json parses JSON body", "[apostol_module]")
{
    HttpRequest req = make_request("POST", "/",
        {{"Content-Type", "application/json"}}, R"({"key":"value"})");
    auto j = EchoModule::content_to_json(req);
    REQUIRE(j.is_object());
    REQUIRE(j["key"] == "value");
}

// ─── PostgreSQL utilities ─────────────────────────────────────────────────────

#ifdef WITH_POSTGRESQL

TEST_CASE("ApostolModule: pg_result_to_json: empty result returns {}", "[apostol_module][pg]")
{
    // PQmakeEmptyPGresult creates a PGresult* with the given status and 0 rows
    PgResult result(PQmakeEmptyPGresult(nullptr, PGRES_TUPLES_OK));

    REQUIRE(EchoModule::pg_result_to_json(result) == "{}");
    REQUIRE(EchoModule::pg_result_to_json(result, "array") == "[]");
    REQUIRE(EchoModule::pg_result_to_json(result, "null")  == "null");
}

TEST_CASE("ApostolModule: pg_result_to_json: empty result with object_name", "[apostol_module][pg]")
{
    PgResult result(PQmakeEmptyPGresult(nullptr, PGRES_TUPLES_OK));
    REQUIRE(EchoModule::pg_result_to_json(result, "", "data") == R"({"data":[]})");
}

TEST_CASE("ApostolModule: reply_pg: empty results vector returns 500", "[apostol_module][pg]")
{
    std::vector<PgResult> empty;
    HttpResponse resp;
    EchoModule::reply_pg(resp, empty);
    std::string s = resp.serialize();
    REQUIRE(s.find("HTTP/1.1 500") != std::string::npos);
    REQUIRE(s.find(R"("code":500)") != std::string::npos);
}

TEST_CASE("ApostolModule: reply_pg: failed result returns 500 with error", "[apostol_module][pg]")
{
    std::vector<PgResult> results;
    results.emplace_back(PQmakeEmptyPGresult(nullptr, PGRES_FATAL_ERROR));
    HttpResponse resp;
    EchoModule::reply_pg(resp, results);
    std::string s = resp.serialize();
    REQUIRE(s.find("HTTP/1.1 500") != std::string::npos);
}

TEST_CASE("ApostolModule: reply_pg: ok empty result returns 200 with {}", "[apostol_module][pg]")
{
    std::vector<PgResult> results;
    results.emplace_back(PQmakeEmptyPGresult(nullptr, PGRES_TUPLES_OK));
    HttpResponse resp;
    EchoModule::reply_pg(resp, results);
    std::string s = resp.serialize();
    REQUIRE(s.find("HTTP/1.1 200 OK") != std::string::npos);
    REQUIRE(s.find("Content-Type: application/json") != std::string::npos);
    REQUIRE(s.find("{}") != std::string::npos);
}

// ─── PostgreSQL utility delegates ─────────────────────────────────────────────

TEST_CASE("ApostolModule: pq_quote_literal escapes strings", "[apostol_module][pg]")
{
    auto q = EchoModule::pq_quote_literal("it's a test");
    REQUIRE(q.find("it''s a test") != std::string::npos);
}

TEST_CASE("ApostolModule: headers_to_json produces JSON object", "[apostol_module][pg]")
{
    std::vector<std::pair<std::string, std::string>> h = {{"Host", "example.com"}, {"Accept", "*/*"}};
    auto j = EchoModule::headers_to_json(h);
    REQUIRE(j.find("\"Host\"") != std::string::npos);
    REQUIRE(j.find("\"example.com\"") != std::string::npos);
}

TEST_CASE("ApostolModule: params_to_json produces JSON object", "[apostol_module][pg]")
{
    std::vector<std::pair<std::string, std::string>> p = {{"id", "42"}, {"name", "test"}};
    auto j = EchoModule::params_to_json(p);
    REQUIRE(j.find("\"id\"") != std::string::npos);
    REQUIRE(j.find("\"42\"") != std::string::npos);
}

TEST_CASE("ApostolModule: check_pg_error parses error JSON", "[apostol_module][pg]")
{
    std::string msg;
    int code = EchoModule::check_pg_error(R"({"error":{"code":40100,"message":"unauthorized"}})", msg);
    REQUIRE(code == 40100);
    REQUIRE(msg == "unauthorized");
}

TEST_CASE("ApostolModule: check_pg_error returns 0 for no error", "[apostol_module][pg]")
{
    std::string msg;
    int code = EchoModule::check_pg_error(R"({"result":"ok"})", msg);
    REQUIRE(code == 0);
}

TEST_CASE("ApostolModule: error_code_to_status maps codes correctly", "[apostol_module][pg]")
{
    REQUIRE(EchoModule::error_code_to_status(40100) == HttpStatus::unauthorized);
    REQUIRE(EchoModule::error_code_to_status(40300) == HttpStatus::forbidden);
    REQUIRE(EchoModule::error_code_to_status(40400) == HttpStatus::not_found);
    REQUIRE(EchoModule::error_code_to_status(500) == HttpStatus::internal_server_error);
    REQUIRE(EchoModule::error_code_to_status(999) == HttpStatus::bad_request);
}

#endif // WITH_POSTGRESQL

// ─── load_allowed_origins ─────────────────────────────────────────────────────

namespace {
/// RAII helper: create a temp dir, remove it on destruction.
struct TempDir {
    std::filesystem::path path;
    explicit TempDir(std::string_view name)
        : path(std::filesystem::temp_directory_path() / name)
    {
        std::filesystem::create_directories(path);
    }
    ~TempDir() { std::filesystem::remove_all(path); }

    /// Write text to a file inside this directory.
    void write(std::string_view filename, std::string_view content) const
    {
        std::ofstream f(path / filename);
        f << content;
    }
};
} // namespace

TEST_CASE("ApostolModule: load_allowed_origins loads javascript_origins from providers", "[apostol_module][cors]")
{
    TempDir dir("apostol_test_oauth2_load");
    dir.write("default.json", R"({
        "web":     {"javascript_origins": ["https://example.com", "http://localhost:3000"]},
        "service": {"javascript_origins": ["https://api.example.com"]}
    })");

    OAuthProviders providers;
    providers.load(dir.path);

    EchoModule mod;
    mod.load_allowed_origins(providers);

    REQUIRE(mod.is_origin_allowed("https://example.com"));
    REQUIRE(mod.is_origin_allowed("http://localhost:3000"));
    REQUIRE(mod.is_origin_allowed("https://api.example.com"));
    REQUIRE_FALSE(mod.is_origin_allowed("https://evil.com"));
}

TEST_CASE("ApostolModule: load_allowed_origins with empty providers is a no-op", "[apostol_module][cors]")
{
    OAuthProviders providers;

    EchoModule mod;
    mod.load_allowed_origins(providers);
    REQUIRE_FALSE(mod.is_origin_allowed("https://example.com"));
}

TEST_CASE("ApostolModule: load_allowed_origins deduplicates origins", "[apostol_module][cors]")
{
    TempDir dir("apostol_test_oauth2_dedup");
    // Both sections list the same origin
    dir.write("a.json", R"({"web":     {"javascript_origins": ["https://example.com"]}})");
    dir.write("b.json", R"({"service": {"javascript_origins": ["https://example.com"]}})");

    OAuthProviders providers;
    providers.load(dir.path);

    EchoModule mod;
    mod.load_allowed_origins(providers);

    // CORS header must appear exactly once
    HttpRequest  req = make_request("GET", "/", {{"Origin", "https://example.com"}});
    HttpResponse resp;
    mod.execute(req, resp);
    const std::string s = resp.serialize();

    const auto first  = s.find("Access-Control-Allow-Origin:");
    const auto second = s.find("Access-Control-Allow-Origin:", first + 1);
    REQUIRE(first  != std::string::npos);
    REQUIRE(second == std::string::npos); // no duplicate
}

TEST_CASE("ApostolModule: load_allowed_origins combined with add_allowed_origin", "[apostol_module][cors]")
{
    TempDir dir("apostol_test_oauth2_mixed");
    dir.write("default.json", R"({"web": {"javascript_origins": ["https://from-file.com"]}})");

    OAuthProviders providers;
    providers.load(dir.path);

    EchoModule mod;
    mod.add_allowed_origin("https://manual.com");
    mod.load_allowed_origins(providers);

    REQUIRE(mod.is_origin_allowed("https://manual.com"));
    REQUIRE(mod.is_origin_allowed("https://from-file.com"));
}
