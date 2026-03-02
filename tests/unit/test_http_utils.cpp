// tests/unit/test_http_utils.cpp
// Unit tests for http_utils: match_path, json_escape, reply_error, url_decode,
// parse_form_body, content_to_json, get_*, redirect, module_config, resolve_path.

#include <catch2/catch_test_macros.hpp>

#include "apostol/http_utils.hpp"
#include "apostol/http.hpp"
#include "apostol/application.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace apostol;

// ─── match_path ─────────────────────────────────────────────────────────────

TEST_CASE("match_path: exact match", "[http_utils]")
{
    std::vector<std::string> pats{"/api/v1", "/status"};
    CHECK(match_path("/api/v1", pats));
    CHECK(match_path("/status", pats));
    CHECK_FALSE(match_path("/api/v2", pats));
    CHECK_FALSE(match_path("/api/v1/extra", pats));
}

TEST_CASE("match_path: wildcard match", "[http_utils]")
{
    std::vector<std::string> pats{"/api/*"};
    CHECK(match_path("/api/anything", pats));
    CHECK(match_path("/api/", pats));
    CHECK(match_path("/api/v1/foo", pats));
    CHECK_FALSE(match_path("/other", pats));
}

TEST_CASE("match_path: empty patterns", "[http_utils]")
{
    std::vector<std::string> pats;
    CHECK_FALSE(match_path("/api/v1", pats));
}

TEST_CASE("match_path: empty pattern entry skipped", "[http_utils]")
{
    std::vector<std::string> pats{"", "/ok"};
    CHECK(match_path("/ok", pats));
    CHECK_FALSE(match_path("/nope", pats));
}

// ─── json_escape ────────────────────────────────────────────────────────────

TEST_CASE("json_escape: clean string", "[http_utils]")
{
    CHECK(json_escape("hello world") == "hello world");
}

TEST_CASE("json_escape: quotes and backslash", "[http_utils]")
{
    CHECK(json_escape(R"(say "hi")") == R"(say \"hi\")");
    CHECK(json_escape("a\\b") == "a\\\\b");
}

TEST_CASE("json_escape: control characters", "[http_utils]")
{
    CHECK(json_escape("line1\nline2") == "line1\\nline2");
    CHECK(json_escape("col1\tcol2") == "col1\\tcol2");
    CHECK(json_escape("cr\rhere") == "cr\\rhere");
}

// ─── reply_error ─────────────────────────────────────────────────────────────

TEST_CASE("reply_error: HttpStatus overload", "[http_utils]")
{
    HttpResponse resp;
    reply_error(resp, HttpStatus::not_found, "page missing");
    auto s = resp.serialize();
    CHECK(s.find("HTTP/1.1 404") != std::string::npos);
    // Body is valid JSON with error code and message
    auto body_start = s.find("\r\n\r\n");
    REQUIRE(body_start != std::string::npos);
    auto body = s.substr(body_start + 4);
    auto j = nlohmann::json::parse(body);
    CHECK(j["error"]["code"] == 404);
    CHECK(j["error"]["message"] == "page missing");
}

TEST_CASE("reply_error: escapes special chars in message", "[http_utils]")
{
    HttpResponse resp;
    reply_error(resp, HttpStatus::bad_request, "bad \"input\"\nnewline");
    auto s = resp.serialize();
    auto body_start = s.find("\r\n\r\n");
    REQUIRE(body_start != std::string::npos);
    auto body = s.substr(body_start + 4);
    // Should parse as valid JSON (json_escape worked)
    auto j = nlohmann::json::parse(body);
    CHECK(j["error"]["message"] == "bad \"input\"\nnewline");
}

TEST_CASE("reply_error: int overload", "[http_utils]")
{
    HttpResponse resp;
    reply_error(resp, 500, "server error");
    auto s = resp.serialize();
    CHECK(s.find("HTTP/1.1 500") != std::string::npos);
    auto body_start = s.find("\r\n\r\n");
    REQUIRE(body_start != std::string::npos);
    auto j = nlohmann::json::parse(s.substr(body_start + 4));
    CHECK(j["error"]["code"] == 500);
}

// ─── url_decode ─────────────────────────────────────────────────────────────

TEST_CASE("url_decode: clean string", "[http_utils]")
{
    CHECK(url_decode("hello") == "hello");
}

TEST_CASE("url_decode: %XX encoding", "[http_utils]")
{
    CHECK(url_decode("hello%20world") == "hello world");
    CHECK(url_decode("%41%42%43") == "ABC");
}

TEST_CASE("url_decode: + as space", "[http_utils]")
{
    CHECK(url_decode("a+b") == "a b");
}

TEST_CASE("url_decode: invalid %XX passthrough", "[http_utils]")
{
    CHECK(url_decode("%GG") == "%GG");
    CHECK(url_decode("%2") == "%2");
}

// ─── parse_form_body ────────────────────────────────────────────────────────

TEST_CASE("parse_form_body: empty body", "[http_utils]")
{
    auto j = parse_form_body("");
    CHECK(j.is_object());
    CHECK(j.empty());
}

TEST_CASE("parse_form_body: multiple params", "[http_utils]")
{
    auto j = parse_form_body("name=John&age=30");
    CHECK(j["name"] == "John");
    CHECK(j["age"] == "30");
}

TEST_CASE("parse_form_body: encoded values", "[http_utils]")
{
    auto j = parse_form_body("msg=hello+world&path=%2Fapi%2Fv1");
    CHECK(j["msg"] == "hello world");
    CHECK(j["path"] == "/api/v1");
}

TEST_CASE("parse_form_body: key without value", "[http_utils]")
{
    auto j = parse_form_body("flag");
    CHECK(j["flag"] == "");
}

// ─── content_to_json ────────────────────────────────────────────────────────

TEST_CASE("content_to_json: JSON body", "[http_utils]")
{
    HttpRequest req;
    req.body = R"({"key":"val"})";
    req.headers.push_back({"Content-Type", "application/json"});

    auto j = content_to_json(req);
    CHECK(j["key"] == "val");
}

TEST_CASE("content_to_json: form body", "[http_utils]")
{
    HttpRequest req;
    req.body = "name=Alice&role=admin";
    req.headers.push_back({"Content-Type", "application/x-www-form-urlencoded"});

    auto j = content_to_json(req);
    CHECK(j["name"] == "Alice");
    CHECK(j["role"] == "admin");
}

TEST_CASE("content_to_json: empty body falls back to params", "[http_utils]")
{
    HttpRequest req;
    req.params.push_back({"q", "search"});

    auto j = content_to_json(req);
    CHECK(j["q"] == "search");
}

// ─── get_* header utilities ─────────────────────────────────────────────────

TEST_CASE("get_host: returns Host header", "[http_utils]")
{
    HttpRequest req;
    req.headers.push_back({"Host", "example.com"});
    CHECK(get_host(req) == "example.com");
}

TEST_CASE("get_host: defaults to localhost", "[http_utils]")
{
    HttpRequest req;
    CHECK(get_host(req) == "localhost");
}

TEST_CASE("get_origin: returns Origin header", "[http_utils]")
{
    HttpRequest req;
    req.headers.push_back({"Origin", "https://example.com"});
    CHECK(get_origin(req) == "https://example.com");
}

TEST_CASE("get_origin: empty when absent", "[http_utils]")
{
    HttpRequest req;
    CHECK(get_origin(req).empty());
}

TEST_CASE("get_protocol: returns X-Forwarded-Proto", "[http_utils]")
{
    HttpRequest req;
    req.headers.push_back({"X-Forwarded-Proto", "https"});
    CHECK(get_protocol(req) == "https");
}

TEST_CASE("get_protocol: defaults to http", "[http_utils]")
{
    HttpRequest req;
    CHECK(get_protocol(req) == "http");
}

TEST_CASE("get_real_ip: returns X-Real-IP", "[http_utils]")
{
    HttpRequest req;
    req.headers.push_back({"X-Real-IP", "1.2.3.4"});
    CHECK(get_real_ip(req) == "1.2.3.4");
}

TEST_CASE("get_user_agent: returns header value", "[http_utils]")
{
    HttpRequest req;
    req.headers.push_back({"User-Agent", "TestBot/1.0"});
    CHECK(get_user_agent(req) == "TestBot/1.0");
}

TEST_CASE("get_user_agent: returns default when absent", "[http_utils]")
{
    HttpRequest req;
    CHECK(get_user_agent(req, "MyDefault") == "MyDefault");
    CHECK(get_user_agent(req).empty());
}

TEST_CASE("get_hostname: returns non-empty string", "[http_utils]")
{
    auto h = get_hostname();
    CHECK_FALSE(h.empty());
}

// ─── redirect ───────────────────────────────────────────────────────────────

TEST_CASE("redirect: sets Location and 302", "[http_utils]")
{
    HttpResponse resp;
    redirect(resp, "https://example.com/new");
    auto s = resp.serialize();
    CHECK(s.find("HTTP/1.1 302") != std::string::npos);
    CHECK(s.find("Location: https://example.com/new") != std::string::npos);
}

TEST_CASE("redirect: custom status code", "[http_utils]")
{
    HttpResponse resp;
    redirect(resp, "/other", HttpStatus::moved_permanently);
    auto s = resp.serialize();
    CHECK(s.find("HTTP/1.1 301") != std::string::npos);
}

// ─── module_config / resolve_path ───────────────────────────────────────────
// These are Application methods. Use a test subclass to access protected settings_.

namespace
{

class TestApp : public Application
{
public:
    TestApp() : Application("test-http-utils")
    {
        settings_.pid_file = std::filesystem::temp_directory_path()
                             / ("apostol-test-hu-" + std::to_string(::getpid()) + ".pid");
    }

    ~TestApp() override
    {
        std::error_code ec;
        std::filesystem::remove(settings_.pid_file, ec);
    }

    void set_prefix(std::string_view p) { settings_.prefix = p; }
};

} // namespace

TEST_CASE("module_config: absent section returns nullptr", "[http_utils]")
{
    TestApp app;
    // No config loaded — should return nullptr safely
    CHECK(app.module_config("PGHTTP") == nullptr);
    CHECK(app.module_config("") == nullptr);
}

TEST_CASE("resolve_path: empty path uses default name", "[http_utils]")
{
    TestApp app;
    app.set_prefix("/opt/apostol/");
    auto p = app.resolve_path("", "files");
    CHECK(p == std::filesystem::path("/opt/apostol/files"));
}

TEST_CASE("resolve_path: absolute path returned as-is", "[http_utils]")
{
    TestApp app;
    app.set_prefix("/opt/apostol/");
    auto p = app.resolve_path("/custom/path");
    CHECK(p == std::filesystem::path("/custom/path"));
}

TEST_CASE("resolve_path: relative path resolved against prefix", "[http_utils]")
{
    TestApp app;
    app.set_prefix("/opt/apostol/");
    auto p = app.resolve_path("data/uploads");
    CHECK(p == std::filesystem::path("/opt/apostol/data/uploads"));
}
